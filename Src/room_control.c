#include "room_control.h"

#include "gpio.h"    // Para controlar LEDs
#include "systick.h" // Para obtener ticks y manejar tiempos
#include "uart.h"    // Para enviar mensajes
#include "tim.h"     // Para controlar el PWM

// Estados de la sala
typedef enum {
    ROOM_IDLE,
    ROOM_OCCUPIED
} room_state_t;

// Variable de estado global
room_state_t current_state = ROOM_IDLE;
static uint32_t led_on_time = 0;
static uint32_t g_door_open = 0;
static volatile uint32_t g_door_open_tick = 0;
static volatile uint32_t g_last_light_button_tick = 0;
static volatile uint8_t g_door_open = 0;
static volatile uint32_t g_last_button_tick = 0;

void room_control_app_init(void)
{

    // Inicializar PWM al duty cycle inicial (estado IDLE -> LED apagado)
    tim3_ch1_pwm_set_duty_cycle(PWM_INITIAL_DUTY);
    uart_send_string("\r\nControlador de Sala v2.0 \n Estado inicial: \n- Lámpara: 20%\n- Puerta cerrada");


}


void room_control_on_button_press(void)
{
    if (current_state == ROOM_IDLE) {
        current_state = ROOM_OCCUPIED;
        tim3_ch1_pwm_set_duty_cycle(100);  // PWM al 100%
        led_on_time = systick_get_ms();
        uart_send_string("Sala ocupada\r\n");
    } else {
        current_state = ROOM_IDLE;
        tim3_ch1_pwm_set_duty_cycle(0);  // PWM al 0%
        uart_send_string("Sala vacía\r\n");
    }
}

void room_control_on_uart_receive(char received_char)
{
    switch (received_char) {
        case 'h':
        case 'H':
            tim3_ch1_pwm_set_duty_cycle(100);
            uart_send_string("PWM: 100%\r\n");
            break;
        case 'l':
        case 'L':
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("PWM: 0%\r\n");
            break;
        case '0':
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("Lámpara apagada.\r\n");
            break;
 
        case 'o':
        case 'O':
            gpio_write_pin(EXTERNO_LED_PWM_PORT, EXTERNO_LED_PWM_PIN, GPIO_PIN_SET);
            g_door_open = 1;
            uart_send_string("Puerta abierta.\r\n");
            break;
        case 'c':
        case 'C':   
            gpio_write_pin(EXTERNO_LED_PWM_PORT, EXTERNO_LED_PWM_PIN, GPIO_PIN_RESET);
            g_door_open = 0;
            uart_send_string("Puerta cerrada.\r\n");
            break;
        case 'I':
        case 'i':
            current_state = ROOM_IDLE;
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("Sala vacía\r\n");
            break;
        case '1':
            tim3_ch1_pwm_set_duty_cycle(10);
            uart_send_string("PWM: 10%\r\n");
            break;
        case '2':
            tim3_ch1_pwm_set_duty_cycle(20);
            uart_send_string("PWM: 20%\r\n");
            break;
        case '3':
            tim3_ch1_pwm_set_duty_cycle(30);
            uart_send_string("PWM: 30%\r\n");
            break;
        case '4':
            tim3_ch1_pwm_set_duty_cycle(40);
            uart_send_string("PWM: 40%\r\n");
            break;
        case '5':
            tim3_ch1_pwm_set_duty_cycle(50);
            uart_send_string("PWM: 50%\r\n");
            break;
        default:
            uart_send_string("Comando desconocido: ");
            uart_send(received_char);
            uart_send_string("\r\n");
            break;
                case '?':
        uart_send_string("Comandos disponibles:\n'1'-'5': Ajustar brillo lámpara (10%, 20%, 30%, 40%, 50%)\n'0': Apagar lámpara\n'o': Abrir puerta(ocupar sala)\n'c': Cerrar puerta(vaciar sala)\n's': Estado del sistema\n'?': Ayuda");
        break;
 
        case 's':
        uart_send_string("Estado:\n- Lámpara: 70%\n- Puerta: Abierta");
        break;
    }
}

void room_control_update(void)
{
    if (current_state == ROOM_OCCUPIED) {
        if (systick_get_ms() - led_on_time >= LED_TIMEOUT_MS) {
            current_state = ROOM_IDLE;
            tim3_ch1_pwm_set_duty_cycle(0);
            uart_send_string("Timeout: Sala vacía\r\n");
        }
    }
}
