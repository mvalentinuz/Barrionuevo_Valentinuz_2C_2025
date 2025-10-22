#include "mh-rd.h"
#include "gpio_mcu.h"

void mhrdInit(mhrd_config_t *config) {
    GPIOInit(config->GPIO, GPIO_INPUT);
}

uint16_t mhrdReadDO(mhrd_config_t *config) {
    return GPIORead(config->GPIO);
}