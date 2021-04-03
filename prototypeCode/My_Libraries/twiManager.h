/*
 *  twiManager.h
 *  
 *  Header file for twiManager.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_TWIMANAGER_H_
#define INC_TWIMANAGER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "app_error.h"
#include "app_util.h"

#include "app_util_platform.h"
#include "nrf_drv_twi.h"

#include "rak.h"

#ifdef DEBUG_TWI
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
 
 // Needed to get the instance ID
#define TWI_INSTANCE_ID     0

// function declarations
bool twiManager_read(uint8_t address, uint8_t * buff, uint8_t numBytes);
bool twiManager_write(uint8_t address, uint8_t * buff, uint8_t numBytes);
void twiManager_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);
bool twiManager_init (void);
#endif
