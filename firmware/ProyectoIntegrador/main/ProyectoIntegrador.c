/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |      SG90      |   EDU-ESP  	|
 * |:--------------:|:-------------:|
 * | 	Vcc 	    |	5V      	|
 * | 	PWM 		| 	GPIO_19		|
 * | 	Gnd 	    | 	GND     	|
 * 
 * |     MH-RD      |   EDU-ESP 	|
 * |:--------------:|:-------------:|
 * | 	AO		    | 	CH0 		|
 * | 	DO  	 	| 	-       	|
 * | 	Gnd 	    | 	GND     	|
 * | 	Vcc 	    |	5V      	|
 * 
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 22/10/2025 | Document creation		                         |
 *
 * @author Barrionuevo Zoe zoe.nicole.barrionuevo@gmail.com y Mauro Valentinuz maurovalentinuz@gmail.com
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "servo_sg90.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "gpio_mcu.h"
#include "switch.h"
#include "led.h"
#include "mh-rd.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_CHECK_PERIOD_US 1000000 // 1 segundo
#define SERVO_PIN GPIO_19  // Para usar PWM1_A
#define SERVO_TENDER SERVO_0           // Número de servo a utilizar
/*==================[internal data definition]===============================*/
/**
 * @brief Indica si está lloviendo (true) o no (false)
 */
bool llueve = false;

/**
 * @brief Handle para la tarea de control del tender
 */
TaskHandle_t controlar_tender_task_handle = NULL;
TaskHandle_t sensarLluvia_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
void FuncTimerA(void* param) {
    vTaskNotifyGiveFromISR(controlar_tender_task_handle, pdFALSE);
    vTaskNotifyGiveFromISR(sensarLluvia_task_handle, pdFALSE);
}

/**
 * @brief Tarea que controla la posición del tender según el estado del clima
 * Mueve el servo a 90° cuando llueve y a 0° cuando no llueve
 */
static void controlar_tender(void *pvParameter)
{
    while (true)
    {
        if (llueve)
        {
            // Si llueve, mover el tender bajo techo (0 grados)
            ServoMove(SERVO_TENDER, 0);
        }
        else
        {
            // Si no llueve, sacar el tender (90 grados)
            ServoMove(SERVO_TENDER, 90);
        }
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // espera la notificación del timer
    }
}

/**
 * @fn void Tecla1()
 * @brief Interrupción de TEC1. Simula cambio en el estado de lluvia
 */
void Tecla1() {
    llueve = !llueve; // Cambiar el estado de lluvia (para pruebas)
}

static void sensarLluvia(void *pvParameter)
{
    uint16_t valor;
    while (true)
    {
        AnalogInputReadSingle(CH0, &valor);
        printf("V%u\n", valor);
        if (valor < 3000)  // Cambiado de 1 a 0
        {
            llueve = true;
            LedOn(LED_3);  // LED para visualizar la detección
            LedOff(LED_1);
        }
        else
        {
            llueve = false;
            LedOff(LED_3);
            LedOn(LED_1);
        }
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}
/*==================[external functions definition]==========================*/
/**
 * @brief Función principal. Inicializa hardware y crea las tareas del sistema
 */
void app_main(void)
{
    // Configuración del timer
    timer_config_t timer_config = {
        .timer = TIMER_A,
        .period = CONFIG_CHECK_PERIOD_US,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    mhrd_config_t mhrd_config = {
        .GPIO = GPIO_2
    };
    analog_input_config_t analogInputConfig = {
		.input = CH0,
		.mode = ADC_SINGLE,
		.sample_frec = 0,
		.func_p = NULL,
		.param_p = NULL};
    AnalogInputInit(&analogInputConfig);
    
    // Inicializaciones
    TimerInit(&timer_config);
    ServoInit(SERVO_TENDER, SERVO_PIN);
    GPIOInit(GPIO_2, GPIO_INPUT);
    LedsInit();
    SwitchesInit();
    SwitchActivInt(SWITCH_1, Tecla1, NULL); // Para simular cambios de lluvia


    // Creación de tareas
    xTaskCreate(&controlar_tender, "Control tender", 4096, NULL, 5, &controlar_tender_task_handle);
    xTaskCreate(&sensarLluvia, "Sensar Lluvia", 4096, NULL, 5, &sensarLluvia_task_handle);

    // Inicio del timer
    TimerStart(timer_config.timer);
}
/*==================[end of file]============================================*/