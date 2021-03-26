/*
 *  rak.h
 *  
 *  Header file for the pin and LED definitions for the RAK WisBLOCK
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_RAK_H_
#define INC_RAK_H_

#include "nrf_gpio.h"

#define RAK_LED_1 	 (NRF_GPIO_PIN_MAP(1, 3))
#define RAK_LED_2		 (NRF_GPIO_PIN_MAP(1, 4))
#define RAK_I2C1_SDA (NRF_GPIO_PIN_MAP(0, 13))
#define RAK_I2C1_SCL (NRF_GPIO_PIN_MAP(0, 14))
#define RAK_I2C2_SDA (NRF_GPIO_PIN_MAP(0, 24))
#define RAK_I2C2_SCL (NRF_GPIO_PIN_MAP(0, 25))

#endif
