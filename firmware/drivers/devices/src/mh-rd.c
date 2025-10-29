#include "mh-rd.h"
#include "gpio_mcu.h"

static mhrd_config_t mhrd_config;

void mhrdInit(mhrd_config_t *config) {
    GPIOInit(config->GPIO, GPIO_INPUT);
    mhrd_config.GPIO = config->GPIO;
}

uint16_t mhrdReadDO() {
    return GPIORead(mhrd_config.GPIO);
}