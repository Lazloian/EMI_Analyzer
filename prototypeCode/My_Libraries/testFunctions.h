/*
 *  testFunctions.h
 *  
 *  Header file for testFunctions.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_TESTFUNCTIONS_H_
#define INC_TESTFUNCTIONS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "sweep.h"
#include "flashManager.h"
#include "usbManager.h"

#include "mem_manager.h"

bool testFunctions_saveDummy (Config * config, bool usb);
bool testFunctions_dummyData(void * buff, uint32_t num_bytes);

#ifdef DEBUG_TEST
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#endif
