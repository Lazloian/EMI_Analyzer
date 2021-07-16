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
 
 #define ADC_MULTIPLIER (double) (0.6 / (1.0 / 6.0)) * (1.0 / 4096.0)
 
 bool adcManager_init(void);
 bool adcManager_channel_init(uint8_t channel, unsigned long input);
 bool adcManager_setBuffer(nrf_saadc_value_t * buff, uint8_t samples);
 bool adcManager_sample(void);
 bool adcManager_sampleNow(uint8_t channel, nrf_saadc_value_t * buff);
 uint8_t adcManager_status(void);
 void saadc_callback(nrf_drv_saadc_evt_t const * p_event);
