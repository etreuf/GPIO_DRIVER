#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "GPIO_1/INCLUDE/GPIO_1.h"






// Definición de pines GPIO
#define S_IN_PIN        GPIO_NUM_4    // Pin para el sensor de entrada
#define S_OUT_PIN       GPIO_NUM_2    // Pin para el sensor de salida
#define TEMCOR_PIN      GPIO_NUM_12   // Pin para el sensor de temperatura corporal
//#define DOOR_PIN        GPIO_NUM_13   // Pin para la cerradura de la puerta
//#define TEMPAMB_PIN     GPIO_NUM_14   // Pin para el sensor de temperatura ambiental
#define FAN_PIN         GPIO_NUM_22   // Pin para el control del ventilador
#define LED_PIN         GPIO_NUM_16   // Pin para el indicador LED
#define BUTTON_PIN      GPIO_NUM_23   // Pin para el botón de encendido/apagado
#define MAX_CAPACITY    5            // Capacidad máxima del establecimiento
#define RED_LED_PIN     GPIO_NUM_19   // Pin para el LED rojo
#define BLUE_LED_PIN    GPIO_NUM_21   // Pin para el LED azul
#define MODE_BUTTON_PIN  GPIO_NUM_36  // Pin para el botón de selección de modo
#define COOL_BUTTON_PIN  GPIO_NUM_13  // Pin para el botón de selección de modo COOL/HEAT

// Variables de estado
bool systemOn = false;
bool doorOpen = false;
int peopleCount = 0;
bool autoMode = true;
bool coolMode = true;
int ambientTemperature;
int mappedambientTemperature;

//Variable para leer temperatura del ambiente
void readTemperatureambient(){
    ambientTemperature = adc1_get_raw(ADC1_CHANNEL_7);  // Leer el valor del ADC para TEMPAMB_PIN
    mappedambientTemperature = (ambientTemperature - 0) * (40 - 1) / (4095 - 0) + 1;
}

// Función para configurar los pines GPIO
void configureGPIO() {
  
    gpio_config_t io_conf;
    // Configurar pines de entrada
    gpio_set_input_isr(S_IN_PIN, GPIO_INTR_DISABLE);
    gpio_set_input_isr(S_OUT_PIN, GPIO_INTR_DISABLE);
    gpio_set_input_isr(TEMCOR_PIN, GPIO_INTR_DISABLE);
    gpio_set_input_isr(BUTTON_PIN, GPIO_INTR_DISABLE);
    gpio_set_input_isr(MODE_BUTTON_PIN, GPIO_INTR_DISABLE);
    gpio_set_input_isr(COOL_BUTTON_PIN, GPIO_INTR_DISABLE);

  

    // Configurar pines de salida
    gpio_reset_pin(FAN_PIN);
    gpio_set_output(FAN_PIN);
    gpio_reset_pin(LED_PIN);
    gpio_set_output(LED_PIN);
    gpio_reset_pin(RED_LED_PIN);
    gpio_set_output(RED_LED_PIN);
    gpio_reset_pin(BLUE_LED_PIN);
    gpio_set_output(BLUE_LED_PIN);
    gpio_config(&io_conf);
    
    // Inicializar pines en estado bajo (apagado)
    
    gpio_set_level(FAN_PIN, 0);
    gpio_set_level(LED_PIN, 0);
    gpio_set_level(RED_LED_PIN, 0);
    gpio_set_level(BLUE_LED_PIN, 0);
}

// Función para configurar el ADC
void configureADC() {
    adc1_config_width(ADC_WIDTH_BIT_12);  // Configurar el ancho de bits de la conversión ADC
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_0);  // Configurar la atenuación y el canal del ADC
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0);  // Configurar la atenuación y el canal del ADC
}

// Función para mostrar el estado del sistema en el terminal
void showSystemStatus() {
    printf("Sistema: %s\n", systemOn ? "ON" : "OFF");
    printf("DOOR: %s\n", doorOpen ? "Open" : "Closed");
    printf("Número de personas: %d\n", peopleCount);
    printf("MODO: %s\n", autoMode ? "Auto" : "On");
    printf("Modo COOL/HEAT: %s\n", coolMode ? "Cool" : "Ceat");
    readTemperatureambient();
    printf("Temperatura ambiente %d\n",mappedambientTemperature);
}

// Función para abrir la puerta durante 5 segundos
void openDoor() {
    doorOpen = true;
    printf("DOOR: %s\n", doorOpen ? "Open" : "Closed");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Esperar 5 segundos
    doorOpen = false;
}

// Función para contar las personas que entran
void countPersonIn() {
    if (!doorOpen && gpio_get_level(S_IN_PIN) == 1) {
        int temperature = adc1_get_raw(ADC1_CHANNEL_4);  // Leer el valor del ADC para TEMCOR_PIN

        // Mapear el valor del ADC al rango 1-40
        int mappedTemperature = (temperature - 0) * (40 - 1) / (4095 - 0) + 1;

        printf("Temperatura corporal. %d\n", mappedTemperature);

        if (peopleCount < MAX_CAPACITY && mappedTemperature >= 34 && mappedTemperature <= 37) {

            openDoor();
            peopleCount++;
            printf("Persona ingresó. Número de personas: %d\n", peopleCount);
            printf("DOOR: %s\n", doorOpen ? "open" : "closed");

        } else if (peopleCount >= MAX_CAPACITY) {

            printf("We are Full, wait..\n");

        } else if (mappedTemperature < 34 || mappedTemperature > 37) {

            // Secuencia de luces rojo-azul para indicar temperatura fuera de rango
            gpio_set_level(RED_LED_PIN, 1);  // Luz roja
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RED_LED_PIN, 0);  // Luz apagada
            gpio_set_level(BLUE_LED_PIN, 1);  // Luz azul
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(BLUE_LED_PIN, 0);  // Luz apagada
            gpio_set_level(RED_LED_PIN, 1);  // Luz roja
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RED_LED_PIN, 0);  // Luz apagada
            gpio_set_level(BLUE_LED_PIN, 1);  // Luz azul
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(BLUE_LED_PIN, 0);  // Luz apagada
            gpio_set_level(RED_LED_PIN, 1);  // Luz roja
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RED_LED_PIN, 0);  // Luz apagada

            
            printf("Temperatura fuera de rango.\n");
        }
    }
}

// Función para contar las personas que salen
void countPersonOut() {
    if (!doorOpen && gpio_get_level(S_OUT_PIN) == 1 && peopleCount > 0) {
        openDoor();
        peopleCount--;

        printf("Persona salió. Número de personas: %d\n", peopleCount);
        printf("DOOR: %s\n", doorOpen ? "open" : "closed");
    }
}


// Función para controlar el ventilador según la configuración
void controlFan(bool autoMode, bool coolMode, int setPoint) {

    readTemperatureambient();
    
    if (autoMode) {
        if (coolMode && mappedambientTemperature > setPoint) {
            gpio_set_level(FAN_PIN, 1);
        } else if (!coolMode && mappedambientTemperature < setPoint) {
            gpio_set_level(FAN_PIN, 1);
        } else {
            gpio_set_level(FAN_PIN, 0);
        }
    } else {
        if (coolMode) {
            gpio_set_level(FAN_PIN, 1);
        } else {
            gpio_set_level(FAN_PIN, 0);
        }
    }
}

// Tarea principal del sistema
void accessControlSystemTask(void *pvParameters) {
    configureGPIO();
    configureADC();
    showSystemStatus();
    
    while (1) {
        if (gpio_get_level(S_IN_PIN) == 1) {
            countPersonIn();
        }
        
        if (gpio_get_level(S_OUT_PIN) == 1) {
            countPersonOut();
        }

 
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarea para controlar el ventilador
void fanControlTask(void *pvParameters) {
    
    int setPoint = 25;      // Punto de ajuste por defecto
    
    while (1) {
        controlFan(autoMode, coolMode, setPoint);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    

}

// Tarea para cambiar el estado del sistema
void changeSystemStateTask(void *pvParameters) {
    bool previousButtonState = false;
    bool currentButtonState;
    bool currentCoolButtonState;
    bool modeButtonState;
    

    while (1) {
        //-----------ON/OFF---------------
        currentButtonState = gpio_get_level(BUTTON_PIN);

        if (currentButtonState != previousButtonState) {
            // Cambiar el estado del sistema
            if (currentButtonState == 1) {
                systemOn = !systemOn;

                if (systemOn) {
                    gpio_set_level(LED_PIN, 1);      // Encender el indicador LED
                    printf("Sistema: ON\n");
                } else {
                    gpio_set_level(LED_PIN, 0);      // Apagar el indicador LED
                    printf("Sistema: OFF\n");
                }

                showSystemStatus();
            }
        }

        previousButtonState = currentButtonState;

        //---------------MODO----------------------
        modeButtonState = gpio_get_level(MODE_BUTTON_PIN);
            
                // Cambiar el modo del sistema
                if (modeButtonState == 1) {
                    
                    
                    autoMode = !autoMode;
                    printf("Modo a cambiado a %s\n", autoMode ? "AUTO" : "ON");
                    
                }
            
        vTaskDelay(pdMS_TO_TICKS(500));
       

      //--------------------------COOL/HEAT----------------------------------
        currentCoolButtonState = gpio_get_level(COOL_BUTTON_PIN);
        
            // Cambiar el modo del sistema
            if (currentCoolButtonState == 1) {
                coolMode = !coolMode;
                printf("Modo COOL/HEAT a cambiado  %s\n", coolMode ? "COOL" : "HEAT");
                
            }       
        
        
        
    }
}
void app_main() {
xTaskCreate(accessControlSystemTask, "accessControlTask", 2048, NULL, 5, NULL);
xTaskCreate(fanControlTask, "fanControlTask", 2048, NULL, 5, NULL);
xTaskCreate(changeSystemStateTask, "changeSystemStateTask", 2048, NULL, 5, NULL);

}