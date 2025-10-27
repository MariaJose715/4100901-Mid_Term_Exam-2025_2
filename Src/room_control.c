/* Src/room_control.c  -- versión corregida y compacta */

#include "room_control.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"
#include "tim.h"

typedef enum { ROOM_IDLE, ROOM_OCCUPIED } room_state_t;

/* Variables globales */
room_state_t current_state = ROOM_IDLE;
static uint8_t current_duty_cycle = PWM_INITIAL_DUTY; // brillo actual
static uint8_t previous_duty_cycle = PWM_INITIAL_DUTY; // para restaurar tras timeout
static uint32_t led_on_time = 0;

/* Para transición gradual 'g' */
static uint8_t step_duty = 255; /* 255 = inactivo, 0..100 valores válidos */
static uint32_t gradual_step_time = 0;

/* Helper para enviar números por UART (0,10,20,...,100) */
static void uart_send_number(uint8_t number)
{
    switch(number) {
        case 0:  uart_send_string("0"); break;
        case 10: uart_send_string("10"); break;
        case 20: uart_send_string("20"); break;
        case 30: uart_send_string("30"); break;
        case 40: uart_send_string("40"); break;
        case 50: uart_send_string("50"); break;
        case 100:uart_send_string("100"); break;
        default: uart_send_string("??"); break;
    }
}

/* Inicialización */
void room_control_app_init(void)
{
    tim3_ch1_pwm_set_duty_cycle(PWM_INITIAL_DUTY);
    current_duty_cycle = PWM_INITIAL_DUTY;
    previous_duty_cycle = PWM_INITIAL_DUTY;
    uart_send_string("Controlador de Sala v2.0\r\n");
    uart_send_string("Estado inicial:\r\n");
    uart_send_string(" - Lámpara: 20%\r\n");
    uart_send_string(" - Puerta: Cerrada\r\n");
}

/* Botón B1: enciende al 100% y comienza timeout */
void room_control_on_button_press(void)
{
    if (current_state == ROOM_IDLE) {
        previous_duty_cycle = current_duty_cycle;
        current_state = ROOM_OCCUPIED;
        current_duty_cycle = 100;
        tim3_ch1_pwm_set_duty_cycle(100);
        led_on_time = systick_get_ms();
        uart_send_string("Sala ocupada\r\n");
    } else {
        /* si estaba ocupada, volver a IDLE y restaurar brillo */
        current_state = ROOM_IDLE;
        current_duty_cycle = previous_duty_cycle;
        tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
        uart_send_string("Sala vacía\r\n");
    }
}

/* Comandos UART */
void room_control_on_uart_receive(char received_char)
{
    switch (received_char) {

        /* Brillo 0..50 (0,1..5 => 0%,10%,..50%) */
        case '0':
            current_duty_cycle = 0;
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("Lámpara apagada.\r\n");
            break;
        case '1': case '2': case '3': case '4': case '5':
            current_duty_cycle = (received_char - '0') * 10;
            tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
            uart_send_string("PWM: ");
            uart_send_number(current_duty_cycle);
            uart_send_string("%\r\n");
            break;

        /* on/off rápido */
        case 'h': case 'H':
            current_duty_cycle = 100;
            tim3_ch1_pwm_set_duty_cycle(100);
            uart_send_string("PWM: 100%\r\n");
            break;
        case 'l': case 'L':
            current_duty_cycle = 0;
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("PWM: 0%\r\n");
            break;

        /* Estado puerta */
        case 'o': case 'O':
            current_state = ROOM_OCCUPIED;
            previous_duty_cycle = current_duty_cycle;
            current_duty_cycle = 100;
            tim3_ch1_pwm_set_duty_cycle(100);
            led_on_time = systick_get_ms();
            uart_send_string("Sala ocupada\r\n");
            break;
        case 'c': case 'C': case 'i': case 'I':
            current_state = ROOM_IDLE;
            tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
            uart_send_string("Sala vacía\r\n");
            break;

        /* Estado actual */
        case 's': case 'S':
            uart_send_string("Estado actual:\r\n - Lámpara: ");
            uart_send_number(current_duty_cycle);
            uart_send_string("%\r\n - Puerta: ");
            uart_send_string(current_state == ROOM_OCCUPIED ? "Abierta\r\n" : "Cerrada\r\n");
            break;

        /* Ayuda */
        case '?':
            uart_send_string("Comandos disponibles:\r\n");
            uart_send_string(" '1'-'5': Ajustar brillo lámpara (10%, 20%, 30%, 40%, 50%)\r\n");
            uart_send_string(" '0'   : Apagar lámpara\r\n");
            uart_send_string(" 'o'   : Abrir puerta (ocupar sala)\r\n");
            uart_send_string(" 'c'   : Cerrar puerta (vaciar sala)\r\n");
            uart_send_string(" 's'   : Estado del sistema\r\n");
            uart_send_string(" 'g'   : Transicion gradual 0%->100%\r\n");
            uart_send_string(" '?'   : Ayuda\r\n");
            break;

        /* Iniciar transición gradual */
        case 'g': case 'G':
            step_duty = 0;
            gradual_step_time = systick_get_ms();
            uart_send_string("Transición gradual iniciada\r\n");
            break;

        /* Default */
        default:
            uart_send_string("Comando desconocido: ");
            uart_send(received_char);
            uart_send_string("\r\n");
            break;
    }
}

/* Actualización periódica (llenar en main loop) */
void room_control_update(void)
{
    uint32_t now = systick_get_ms();

    /* Timeout: si ocupada, después de LED_TIMEOUT_MS restaurar */
    if (current_state == ROOM_OCCUPIED && (now - led_on_time) >= LED_TIMEOUT_MS) {
        current_state = ROOM_IDLE;
        current_duty_cycle = previous_duty_cycle;
        tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
        uart_send_string("Timeout: Sala vacía (brillo restaurado)\r\n");
    }

    /* Transición gradual: subir 10% cada 500 ms hasta >100 */
    if (step_duty != 255) {
        if ((now - gradual_step_time) >= 500) {
            if (step_duty <= 100) {
                tim3_ch1_pwm_set_duty_cycle(step_duty);
                uart_send_string("PWM gradual\r\n");
            }
            step_duty += 10;
            gradual_step_time = now;
            if (step_duty > 100) {
                /* finalizar */
                step_duty = 255;
                tim3_ch1_pwm_set_duty_cycle(current_duty_cycle); /* restaurar */
                uart_send_string("Transición gradual completada\r\n");
            }
        }
    }
}
