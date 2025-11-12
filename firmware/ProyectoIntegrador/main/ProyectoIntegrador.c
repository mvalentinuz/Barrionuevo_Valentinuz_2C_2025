/*! @mainpage Proyecto Integrador - Tendedero Automatizado
 *
 * @section genDesc General Description
 * El proyecto consiste en el desarrollo de un tendedero de ropa automatizado controlado por microcontrolador, 
 * capaz de gestionar de forma inteligente el momento de extender o resguardar la ropa según las condiciones ambientales. 
 * Mediante un sensor de lluvia, el sistema detecta la presencia de lluvia y, en caso de ser necesario, 
 * acciona un mecanismo motorizado (servomotor) que retrae la ropa hacia el interior para evitar que se moje. 
 * El sistema también cuenta con un módulo Bluetooth que envía notificaciones al teléfono móvil, informando 
 * al usuario sobre los eventos de guardado o despliegue.
 * 
 * <a href="https://drive.google.com/file/d/1KxaF9nzWENHfq2OSz06COwE2RX_ZmBMj/view?usp=sharing">Operation Example</a>
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
 * | 	AO		    | 	CH0 		|//AO salida analogica 0 a 3300mv
 * | 	DO  	 	| 	-       	|//DO salida digital 0 o 1
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
 * | 12/11/2025 | Finalización del proyecto		                 |
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
#include "ble_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_CHECK_PERIOD_US 1000000  // 1 segundo
#define SERVO_PIN GPIO_19  // Para usar PWM1_A
#define SERVO_TENDER SERVO_0  // Número de servo a utilizar
/*==================[internal data definition]===============================*/
/**
 * @brief Indica si está lloviendo (true) o no (false)
 */
bool llueve = false;

/**
 * @brief Handle para la tarea de control del tender
 */
TaskHandle_t controlar_tender_task_handle = NULL;

/**
 * @brief Handle para la tarea de sensado de lluvia
 */
TaskHandle_t sensarLluvia_task_handle = NULL;

/**
 * @brief Handle para la tarea de Bluetooth
 */
TaskHandle_t bluetooth_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @fn void FuncTimerA(void* param)
 * @brief Función del timer que notifica a las tareas correspondientes
 */
void FuncTimerA(void* param) {
    vTaskNotifyGiveFromISR(controlar_tender_task_handle, pdFALSE);
    vTaskNotifyGiveFromISR(sensarLluvia_task_handle, pdFALSE);
}

/**
 * @fn void controlar_tender(void *pvParameter)
 * @brief Tarea que controla la posición del tender según el estado del clima. 
 * Mueve el servo a 90° cuando no llueve y a 0° cuando llueve
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

/**
 * @fn static void sensarLluvia(void *pvParameter)
 * @brief Tarea que sensa la lluvia usando el sensor MH-RD
 */
static void sensarLluvia(void *pvParameter)
{
    uint16_t valor;
    while (true)
    {
        AnalogInputReadSingle(CH0, &valor); //conversión analógica digital
        printf("Valor: %u\n", valor);
        if (valor < 3000)  // valores de 0 a 3300 mv
        {
            llueve = true;
            LedOn(LED_3); // prende el verde
            LedOff(LED_1);
        }
        else
        {
            llueve = false;
            LedOff(LED_3);
            LedOn(LED_1); // prende el rojo
        }
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

/**
 * @fn void bluetooth(*pvParameter)
 * @brief Tarea que envía el estado de lluvia por Bluetooth
 */
void bluetooth(void *pvParameter){
    char buffer[30];
    while(true){
        if(llueve){
            sprintf(buffer, "*NEstado: Llueve\n");
            BleSendString(buffer);
        }
        else{
            sprintf(buffer, "*NEstado: No Llueve\n");
            BleSendString(buffer);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
    // Configuración del ADC
    analog_input_config_t analogInputConfig = {
		.input = CH0,
		.mode = ADC_SINGLE,
		.sample_frec = 0,
		.func_p = NULL,
		.param_p = NULL};
    //Configuración del BLE
    ble_config_t BLEconfig = {
        "AppTender",
        BLE_NO_INT
    };
    // Inicializaciones
    AnalogInputInit(&analogInputConfig);
    TimerInit(&timer_config);
    ServoInit(SERVO_TENDER, SERVO_PIN);
    LedsInit();
    SwitchesInit();
    SwitchActivInt(SWITCH_1, Tecla1, NULL); // Para simular cambios de lluvia
    BleInit(&BLEconfig);

    // Creación de tareas
    xTaskCreate(&controlar_tender, "Control tender", 4096, NULL, 5, &controlar_tender_task_handle);
    xTaskCreate(&sensarLluvia, "Sensar Lluvia", 4096, NULL, 5, &sensarLluvia_task_handle);
    xTaskCreate(&bluetooth, "Bluetooth", 4096, NULL, 5, &bluetooth_task_handle);

    // Inicio del timer
    TimerStart(timer_config.timer);
}
/*==================[end of file]============================================*/