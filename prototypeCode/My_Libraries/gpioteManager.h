/*
 *  gpioteManager.h
 *  
 *  Header file for gpioteManager.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_GPIOTEMANAGER_H_
#define INC_GPIOTEMANAGER_H_

#include "nrf_drv_gpiote.h"
#include "app_error.h"

// eventaully remove these
#include "boards.h"
#include "rtcManager.h"

#ifdef DEBUG_GPIOTE
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define BUTTON_START (BSP_BUTTON_0) // Button 1 starts rtc
#define BUTTON_STOP  (BSP_BUTTON_1) // Button 2 stops rtc

#define LED_RTC    (BSP_LED_0) // LED to signal if RTC is on
#define LED_USB    (BSP_LED_1) // LED to signal if USB if connected
#define LED_SWEEP  (BSP_LED_2) // LED to signal if sweep is being done
#define LED_AD5933 (BSP_LED_3) // LED to singal if the AD5933 is connected

bool gpioteManager_getEvent(int * buff);
bool gpioteManager_writePin(uint32_t pin, uint8_t val);
bool gpioteManager_init(void);
static void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void gpiote_init(void);
#endif
