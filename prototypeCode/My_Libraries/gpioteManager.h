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

#include "rak.h"

#ifdef DEBUG_GPIOTE
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

bool gpioteManager_getEvent(int * buff);
bool gpioteManager_writePin(uint32_t pin, uint8_t val);
bool gpioteManager_init(void);
static void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void gpiote_init(void);
#endif
