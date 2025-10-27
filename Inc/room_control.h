#ifndef ROOM_CONTROL_H
#define ROOM_CONTROL_H

#include <stdint.h>

// -------------------------------------------------------------
// Configuración general
// -------------------------------------------------------------

#define LED_TIMEOUT_MS     10000  // Tiempo (ms) para restaurar el brillo previo (10 s)
#define PWM_INITIAL_DUTY   20     // Duty cycle inicial (20%)

// -------------------------------------------------------------
// Prototipos de funciones públicas
// -------------------------------------------------------------

/**
 * @brief Inicializa la lógica de control de sala.
 *        Configura el estado inicial, PWM y mensaje de bienvenida.
 */
void room_control_app_init(void);

/**
 * @brief Maneja la pulsación del botón B1.
 *        Cambia el estado de la sala (encendido temporal de 100%).
 */
void room_control_on_button_press(void);

/**
 * @brief Procesa un carácter recibido por UART (comandos del usuario).
 * @param received_char Carácter recibido.
 */
void room_control_on_uart_receive(char received_char);

/**
 * @brief Actualiza el estado del sistema de forma periódica.
 *        Maneja timeouts y transiciones graduales.
 */
void room_control_update(void);

#endif // ROOM_CONTROL_H
