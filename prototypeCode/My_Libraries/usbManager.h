/*
 *  usbManager.h
 *
 *  Header file for usbManager.c
 *
 *  Author: Henry Silva
 *
 */

#ifndef INC_USBMANAGER_H_
#define INC_USBMANAGER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"

#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "app_timer.h"

#include "boards.h"

#include "AD5933.h"

#ifdef DEBUG_USB
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

// if power detection is enabled
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

// LED for USB status
#define LED_USB_RESUME      1

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define READ_SIZE 1
#define WRITE_SIZE 32

bool usbManager_sendSweep(uint32_t * freq, uint16_t * real , uint16_t * imag, MetaData * metadata);
bool usbManager_getByte(uint8_t * buff);
bool usbManager_writeBytes(void * buff, uint32_t numBytes);
bool usbManager_readBytes(void * buff, uint32_t numBytes);
bool usbManager_readReady(void);
bool usbManager_init(void);
void usbManager_flush(void);

void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);
void usbd_user_ev_handler(app_usbd_event_type_t event);
static void init_usb(void);

#endif
