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

// standard includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#ifdef DEBUG_TASKS
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

// My Libraries
#include "twiManager.h"
#include "AD5933.h"
#include "flashManager.h"
#include "usbManager.h"
#include "gpioteManager.h"
#include "sensorFunctions.h"
#include "rak.h"

// Testing
#include "testFunctions.h"

#define LED_SWEEP (RAK_LED_1)
#define LED_BLINK (RAK_LED_2)

#define SEC_TO_TICK(X) (int)(X * ((float) 1000 / portTICK_PERIOD_MS)) // macro to convert seconds to ticks
#define MS_TO_TICK(X)  (int)(X * ((float) 1 / portTICK_PERIOD_MS))

#define SWEEP_PERIOD 1800   // period between sweeps in seconds
#define INPUT_PERIOD 250	 // period between input checks in milliseconds
#define BLINK_PERIOD 10   // period between RTC_LED blinks in seconds
#define USB_PERIOD   500 // period between usb checks in millisenconds

#define START_DELAY	 3 // delay in seconds for sweeps to start after power on. At least a second is needed for usb

// Tasks
void sensorTasks_input(void * pvParameter);
void sensorTasks_sweep(void * pvParameter);
void sensorTasks_blink(void * pvParameter);
void sensorTasks_usb(void * pvParameter);

// Idle hook
void vApplicationIdleHook(void);

// Init
bool sensorTasks_init(void);

#endif
