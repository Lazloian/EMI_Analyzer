/*
 *  adcManager.h
 *
 *  Header for adcManager.c
 *
 *  Author: Henry Silva
 *
 */
 
 #include "nrf_drv_saadc.h"
 
 #ifdef DEBUG_ADC
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
 
 #define ADCMANAGER_BUSY				0
 #define ADCMANAGER_READY				1
 
 bool adcManager_init(void);
 bool adcManager_channel_init(uint8_t channel, unsigned long input);
 bool adcManager_setBuffer(int16_t * buff, uint8_t samples);
 bool adcManager_sample(void);
 uint8_t adcManager_status(void);
 static void saadc_callback(nrf_drv_saadc_evt_t const * p_event);
