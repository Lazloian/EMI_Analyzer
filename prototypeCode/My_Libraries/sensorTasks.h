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

#define BUTTON_START (BSP_BUTTON_0) // Button 1 starts rtc
#define BUTTON_STOP  (BSP_BUTTON_1) // Button 2 stops rtc

#define LED_RTC    (BSP_LED_0) // LED to signal if RTC is on
#define LED_USB    (BSP_LED_1) // LED to signal if USB if connected
#define LED_SWEEP  (BSP_LED_2) // LED to signal if sweep is being done
#define LED_AD5933 (BSP_LED_3) // LED to singal if the AD5933 is connected

#define SEC_TO_TICK(X) (int)(X * ((float) 1000 / portTICK_PERIOD_MS)) // macro to convert seconds to ticks
#define MS_TO_TICK(X)  (int)(X * ((float) 1 / portTICK_PERIOD_MS))

#define SWEEP_PERIOD 1800   // period between sweeps in seconds
#define INPUT_PERIOD 250	 // period between input checks in milliseconds
#define BLINK_PERIOD 10   // period between RTC_LED blinks in seconds
#define USB_PERIOD   500 // period between usb checks in millisenconds

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
