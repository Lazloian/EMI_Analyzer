/*
 *  adcManager.c
 *
 *  Inits and control the functions of the SAADC on the NRF52840
 *
 *  Author: Henry Silva
 *
 */

#include "adcManager.h"

static volatile uint8_t status = ADCMANAGER_BUSY;
static uint8_t channels_enabled = 0;
static bool buffer_set = false;

// Returns the current status of the adc
uint8_t adcManager_status(void)
{
	return status;
}

// Initiates a sampling of all enabled channels on the saadc
// Return value:
//	false: if sample start fail
//	true : if sample start success
bool adcManager_sample(void)
{
	ret_code_t error; // stores error
	
	// check if buffer has been set or if the saadc is busy
	if (!buffer_set || status == ADCMANAGER_BUSY)
	{
#ifdef DEBUG_ADC
		if (!buffer_set) NRF_LOG_INFO("ADC: Buffer not set, start sample fail");
		else NRF_LOG_INFO("ADC: Sample already running, start sample fail");
		NRF_LOG_FLUSH();
#endif
		return false;
	}
	
	// start sampling
	error = nrf_drv_saadc_sample();
	APP_ERROR_CHECK(error);
	
	// check error
	if (error == NRF_SUCCESS)
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: Sampling started");
		NRF_LOG_FLUSH();
#endif
		status = ADCMANAGER_BUSY;
		return true;
	}
	else
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: Sampling start fail");
		NRF_LOG_FLUSH();
#endif
		return false;
	}
}

// Prepares the channels for sampling by setting up the data buffer
// Note: The size of the buffer (number of samples) must be a multiple of the number of channels enabled
// Arguments:
//	* buff : pointer to the buffer to store the data
//	samples: the number of samples to store in the buffer
// Return value:
//	false: if buffer set error
//	true : if buffer set success
 bool adcManager_setBuffer(int16_t * buff, uint8_t samples)
 {
	 ret_code_t error; // stores errors
	 
	 // check if not channels are enabled or samples is not a multiple of enabled channels
	 if (!channels_enabled || samples % channels_enabled)
	 {
#ifdef DEBUG_ADC
		 if (!channels_enabled) NRF_LOG_INFO("ADC: No channels enabled, buffer init fail");
		 else NRF_LOG_INFO("ADC: Samples not a multiple of enabled channels, buffer init fail");
		 NRF_LOG_FLUSH();
#endif
		 return false;
	 }
	 
	 // set up buffer
	 error = nrf_drv_saadc_buffer_convert((nrf_saadc_value_t *) buff, samples);
   APP_ERROR_CHECK(error);
	 
	 // error check
	 if (error == NRF_SUCCESS)
	 {
#ifdef DEBUG_ADC
		 NRF_LOG_INFO("ADC: Buffer init success");
		 NRF_LOG_FLUSH();
#endif
		 buffer_set = true;
		 return true;
	 }
	 else
	 {
#ifdef DEBUG_ADC
		 NRF_LOG_INFO("ADC: Buffer init fail");
		 NRF_LOG_FLUSH();
#endif
		 return false;
	 }
 }
 
// Inits a channel on the saadc
// Arguments: 
//	channel: the channel number to use (0 - 7)
//	input  : the analog input pin (0 - 7) to use for this channel, example: NRF_SAADC_INPUT_AIN0
// Return value:
//  false: if channel init error
//  true : if channel init success
bool adcManager_channel_init(uint8_t channel, unsigned long input)
{
	ret_code_t err_code; // to store error
	
	// set up the default channel config
	nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(input);
	
	// init the channel
	err_code = nrf_drv_saadc_channel_init(channel, &config);
  APP_ERROR_CHECK(err_code);
	
	// error check
	if (err_code == NRF_SUCCESS)
	{
		channels_enabled++;
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: Channel %d enabled", channel);
		NRF_LOG_FLUSH();
#endif
		return true;
	}
	else
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: Channel %d init fail", channel);
		NRF_LOG_FLUSH();
#endif
		return false;
	}
}

// Inits the saadc in the NRF52
// Return value:
//  false if init error
//  true  if init success
bool adcManager_init(void)
{
	ret_code_t err_code; // to store the error code
	
	// set the config options for the saadc
	nrf_drv_saadc_config_t config = {
    .resolution         = (nrf_saadc_resolution_t) 2, 	 // 12 bit resolution
    .oversample         = (nrf_saadc_oversample_t) 0,		 // oversampling disabled
    .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY,
    .low_power_mode     = 1															 // low power mode enabled
	};
	
	// init saadc
	err_code = nrf_drv_saadc_init(&config, saadc_callback);
	APP_ERROR_CHECK(err_code);
	
	if (err_code == NRF_SUCCESS)
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: ADC Init Success");
		NRF_LOG_FLUSH();
#endif
		return true;
	}
	else
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: ADC Init Fail");
		NRF_LOG_FLUSH();
#endif
		return false;
	}
}

// Callback function for SAADC events
static void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
	{
#ifdef DEBUG_ADC
		NRF_LOG_INFO("ADC: Sample Finished");
		NRF_LOG_FLUSH();
#endif
		
		status = ADCMANAGER_READY;
	}
}
