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
#include "rak.h"

#include "mem_manager.h"

#ifdef DEBUG_FUNCTIONS
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

bool sensorFunctions_deleteSweeps(uint32_t numSaved, bool usb);
bool sensorFunctions_sendConfig(Sweep * sweep, uint32_t * numSaved);
void sensorFunctions_set_default(Sweep * sweep);
bool sensorFunctions_sendSweep(uint32_t numSaved);
bool sensorFunctions_sweepAndSend(Sweep * sweep);
bool sensorFunctions_saveSweep(Sweep * sweep, uint32_t * numSaved, bool usb);
bool sensorFunctions_init(void);

#endif
