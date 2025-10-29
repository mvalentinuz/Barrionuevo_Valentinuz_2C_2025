#ifndef MHRD_H
#define MHRD_H

#include <stdint.h>
#include "gpio_mcu.h"

// Struct de configuración del sensor de lluvia
typedef struct {
    uint8_t GPIO;
} mhrd_config_t;

// Funciones para inicializar y leer el sensor de lluvia
void mhrdInit(mhrd_config_t *config);
uint16_t mhrdReadDO();

#endif