/*
 *  rtcManager.h
 *  
 *  Header file for rtcManager.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_RTCMANAGER_H_
#define INC_RTCMANAGER_H_

#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "app_error.h"

#ifdef DEBUG_RTC
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define RTC_FREQ 8 																 // RTC frequency in Hz
#define PRESCALER RTC_FREQ_TO_PRESCALER(RTC_FREQ) // prescaler for RTC_FREQ
#define START_TIME 5														 // seconds to first compare after rtc enabled
#define COMPARE_TIME 1800							    		  // number of seconds between compare events

bool rtcManager_enable(void);
bool rtcManager_disable(void);
bool rtcManager_checkCompare(void);
bool rtcManager_init(void);
static void rtc_handler(nrf_drv_rtc_int_type_t int_typestatic);
static void lfclk_config(void);
static void rtc_config(void);

#endif
