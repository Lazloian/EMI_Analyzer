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
#include <math.h>

#include "AD5933.h"
#include "twiManager.h"
#include "sweep.h"
#include "flashManager.h"
#include "usbManager.h"
#include "gpioteManager.h"
#include "rak.h"
#include "bleManager.h"
#include "adcManager.h"

#include "mem_manager.h"

#ifdef TESTING
#include "testFunctions.h"
#endif

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "nrf_sdh_freertos.h"

#ifdef DEBUG_FUNCTIONS
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

// variables used in ntc temperature calculations
#define TEMP_A 0.0016
#define TEMP_B 0.00024298
#define TEMP_C 0.00000011795

#define K_TO_F(X) ((X - 273.15) * 1.8) + 32
#define K_TO_C(X) (X - 273.15)
#define C_TO_F(X) (X * 1.8) + 32

bool sensorFunctions_getTemp(double * temp);
bool sensorFunctions_deleteSweeps(Config * config, bool usb);
bool sensorFunctions_sendNumSweeps(Config * config);
void sensorFunctions_set_default(Sweep * sweep);
bool sensorFunctions_sendSweep(uint32_t numSaved);
bool sensorFunctions_sweepAndSend(Sweep * sweep);
bool sensorFunctions_saveSweep(Config * config, bool usb);
bool sensorFunctions_init(void);

#endif
