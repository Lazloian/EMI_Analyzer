/*
 *  gpioteManager.c
 *
 *  Controls gpiote.
 *
 *  Author: Henry Silva
 *
 */

#include "gpioteManager.h"

bool gpioteManager_writePin(uint32_t pin, uint8_t val)
{
	if (val)
	{
		nrf_drv_gpiote_out_set(pin);
	}
	else
	{
		nrf_drv_gpiote_out_clear(pin);
	}
	return true;
}

bool gpioteManager_init(void)
{
	gpiote_init();
	return true;
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	// check which button
	if (pin == BUTTON_START && action == GPIOTE_CONFIG_POLARITY_HiToLo)
	{
		rtcManager_enable();
		nrf_drv_gpiote_out_clear(LED_RTC);
	}
	else if (pin == BUTTON_STOP && action == GPIOTE_CONFIG_POLARITY_HiToLo)
	{
		rtcManager_disable();
		nrf_drv_gpiote_out_set(LED_RTC);
	}
}

static void gpiote_init(void)
{
	ret_code_t err_code;

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

	// set up LED output, false means that its starts low
	nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(true);

	// set up RTC LED
	err_code = nrf_drv_gpiote_out_init(LED_RTC, &out_config);
	APP_ERROR_CHECK(err_code);
	
	// set up SWEEP LED
	err_code = nrf_drv_gpiote_out_init(LED_SWEEP, &out_config);
	APP_ERROR_CHECK(err_code);
	
	// set up SWEEP AD5933
	err_code = nrf_drv_gpiote_out_init(LED_AD5933, &out_config);
	APP_ERROR_CHECK(err_code);

	// set up button input, true means that the interrupt is enabled
	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
	in_config.pull = NRF_GPIO_PIN_PULLUP; // buttons are low when enabled so pull up is needed

	// set up start button
	err_code = nrf_drv_gpiote_in_init(BUTTON_START, &in_config, in_pin_handler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(BUTTON_START, true);
	
	// set up stop button
	err_code = nrf_drv_gpiote_in_init(BUTTON_STOP, &in_config, in_pin_handler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(BUTTON_STOP, true);
}
