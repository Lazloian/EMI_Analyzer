/*
 *  sensorTasks.h
 *  
 *  Header file for sensorTasks.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_SENSORTASKS_H_
#define INC_SENSORTASKS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "twiManager.h"
#include "AD5933.h"
#include "flashManager.h"
#include "usbManager.h"
#include "rtcManager.h"
#include "gpioteManager.h"

#include "mem_manager.h"

#ifdef DEBUG_AD5933
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

bool sensorTasks_sendConfig(Sweep * sweep, uint32_t * numSaved);
void sensorTasks_set_default(Sweep * sweep);
bool sensorTasks_sendSweep(uint32_t numSaved);
bool sensorTasks_sweepAndSend(Sweep * sweep);
bool sensorTasks_saveSweep(Sweep * sweep, uint32_t * numSaved, bool usb);
bool sensorTasks_init(void);

#endif
