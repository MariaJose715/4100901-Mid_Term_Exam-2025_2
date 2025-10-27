#include "room_control.h"

#include "gpio.h"    // Control de LEDs
#include "systick.h" // Control de tiempos
#include "uart.h"    // Comunicación serie
#include "tim.h"     // PWM

// Estados de la sala
typedef enum {
    ROOM_IDLE,
    ROOM_OCCUPIED
} room_state_t;

// Variables globales
room_state_t current_state = ROOM_IDLE;
static uint8_t current_duty_cycle = PWM_INITIAL_DUTY;
static uint32_t led_on_time = 0;
static uint8_t previous_duty_cycle = PWM_INITIAL_DUTY; // Para restaurar brillo tras 10 s
static uint8_t step_duty = 0;                          // Para transición gradual
static uint32_t gradual_step_time = 0;                 // Tiempo entre pasos de transición

// -------------------------------------------------------------
// Inicialización del sistema
// -------------------------------------------------------------
void room_control_app_init(void)
{
    tim3_ch1_pwm_set_duty_cycle(PWM_INITIAL_DUTY); // Inicializa PWM al 20%
    uart_send_string("Controlador de Sala v2.0\r\n");
    uart_send_string("Estado inicial:\r\n");
    uart_send_string(" - Lámpara: 20%\r\n");
    uart_send_string(" - Puerta: Cerrada\r\n");
}

// -------------------------------------------------------------
// Evento: botón presionado
// - Enciende al 100% por 10 s y luego restaura el brillo previo
// -------------------------------------------------------------
void room_control_on_button_press(void)
{
    if (current_state == ROOM_IDLE) {
        previous_duty_cycle = current_duty_cycle; // Guardar brillo actual
        current_state = ROOM_OCCUPIED;
        current_duty_cycle = 100;
        tim3_ch1_pwm_set_duty_cycle(100);
        led_on_time = systick_get_ms();
        uart_send_string("Sala ocupada (100%)\r\n");
    } else {
        current_state = ROOM_IDLE;
        current_duty_cycle = previous_duty_cycle;
        tim3_ch1_pwm_set_duty_cycle(previous_duty_cycle);
        uart_send_string("Sala vacía\r\n");
    }
}

// -------------------------------------------------------------
// Evento: comando recibido por UART
// -------------------------------------------------------------
void room_control_on_uart_receive(char c)
{
    switch (c) {
        // --- Control manual de brillo ---
        case '0': case '1': case '2': case '3': case '4': case '5': {
            current_duty_cycle = (c - '0') * 10; // 0→0%, 1→10%, ...
            tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
            uart_send_string("PWM: ");
            uart_send_uint8(current_duty_cycle);
            uart_send_string("%\r\n");
            break;
        }

        // --- Encender/apagar manualmente ---
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

        // --- Cambiar estado de sala ---
        case 'o': case 'O':
            current_state = ROOM_OCCUPIED;
            tim3_ch1_pwm_set_duty_cycle(100);
            uart_send_string("Puerta abierta / Sala ocupada\r\n");
            break;

        case 'c': case 'C': case 'i': case 'I':
            current_state = ROOM_IDLE;
            tim3_ch1_pwm_set_duty_cycle(current_duty_cycle);
            uart_send_string("Puerta cerrada / Sala vacía\r\n");
            break;

        // --- Transición gradual de 0 a 100% ---
        case 'g': case 'G':
            step_duty = 0;
            gradual_step_time = systick_get_ms();
            uart_send_string("Transición gradual iniciada\r\n");
            break;

        // --- Mostrar estado actual ---
        case 's': case 'S':
            uart_send_string("Estado actual:\r\n - Lámpara: ");
            uart_send_uint8(current_duty_cycle);
            uart_send_string("%\r\n - Puerta: ");
            uart_send_string(current_state == ROOM_OCCUPIED ? "Abierta\r\n" : "Cerrada\r\n");
            break;

        // --- Mostrar menú de ayuda ---
        case '?':
            uart_send_string("Comandos disponibles:\r\n");
            uart_send_string(" '1'-'5': Ajustar brillo (10%-50%)\r\n");
            uart_send_string(" '0'   : Apagar lámpara\r\n");
            uart_send_string(" 'o'   : Abrir puerta (ocupar sala)\r\n");
            uart_send_string(" 'c'   : Cerrar puerta (vaciar sala)\r\n");
            uart_send_string(" 'g'   : Transición gradual 0%-100%\r\n");
            uart_send_string(" 's'   : Estado del sistema\r\n");
            uart_send_string(" '?'   : Ayuda\r\n");
            break;

        default:
            uart_send_string("Comando desconocido: ");
            uart_send(c);
            uart_send_string("\r\n");
            break;
    }
}

// -------------------------------------------------------------
// Actualización periódica (llamada desde main loop)
// -------------------------------------------------------------
void room_control_update(void)
{
    uint32_t now = systick_get_ms();

    // 1. Timeout: restaurar brillo previo tras 10 s en estado ocupado
    if (current_state == ROOM_OCCUPIED && now - led_on_time >= LED_TIMEOUT_MS) {
        current_state = ROOM_IDLE;
        tim3_ch1_pwm_set_duty_cycle(previous_duty_cycle);
        current_duty_cycle = previous_duty_cycle;
        uart_send_string("Timeout: brillo restaurado\r\n");
    }

    // 2. Transición gradual: subir 10% cada 500 ms hasta 100%
    if (step_duty <= 100 && (now - gradual_step_time) >= 500) {
        tim3_ch1_pwm_set_duty_cycle(step_duty);
        step_duty += 10;
        gradual_step_time = now;
        if (step_duty > 100)
            uart_send_string("Transición completada\r\n");
    }
}
