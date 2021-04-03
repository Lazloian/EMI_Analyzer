/*
 *  sensorFunctions.h
 *  
 *  Header file for sensorFunctions.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_SENSORFUNCTIONS_H_
#define INC_SENSORFUNCTIONS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "AD5933.h"
#include "twiManager.h"
#include "sweep.h"
#include "flashManager.h"
#include "usbManager.h"
#include "gpioteManager.h"

#include "mem_manager.h"

#ifdef DEBUG_FUNCTIONS
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

bool sensorFunctions_sendConfig(Sweep * sweep, uint32_t * numSaved);
void sensorFunctions_set_default(Sweep * sweep);
bool sensorFunctions_sendSweep(uint32_t numSaved);
bool sensorFunctions_sweepAndSend(Sweep * sweep);
bool sensorFunctions_saveSweep(Sweep * sweep, uint32_t * numSaved, bool usb);
bool sensorFunctions_init(void);

#endif
