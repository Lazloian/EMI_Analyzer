/*
 *  rtcManager.c
 *
 *  Controls the rtc.
 *
 *  Author: Henry Silva
 *
 */
 
 #include "rtcManager.h"
 
static const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2); // Declaring an instance of nrf_drv_rtc for RTC2.
static uint32_t compares = 1; 													 // To keep track of the number of compare events triggered
static volatile bool compare_triggered = false; 				// Saves if a compare event has been triggered

bool rtcManager_enable(void)
{
	nrf_drv_rtc_enable(&rtc);
	return true;
}

bool rtcManager_disable(void)
{
	nrf_drv_rtc_disable(&rtc);
	return true;
}

bool rtcManager_checkCompare(void)
{
	if (compare_triggered)
	{
		compare_triggered = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool rtcManager_init(void)
{
	// check if the low frequency clock is enabled, if not enable it
	if (!nrf_drv_clock_lfclk_is_running())
	{
		lfclk_config();
	}
	
	// init and configure the rtc
	rtc_config();
	
	return true;
}

// Handler for RTC interrupts like the tick and compare
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
	uint32_t err_code;
	
	// check if compare or tick event
	if (int_type == NRF_DRV_RTC_INT_COMPARE0)
	{
		err_code = nrf_drv_rtc_cc_set(&rtc, 0, (compares * (COMPARE_TIME * RTC_FREQ)) + (START_TIME * RTC_FREQ), true);
		
		if (err_code != NRF_SUCCESS)
		{
#ifdef DEBUG_RTC
			NRF_LOG_INFO("RTC: Compare set fail");
			NRF_LOG_FLUSH();
#endif
		}
	  compares++;
		compare_triggered = true;
	}
	// currently not being used
	else if (int_type == NRF_DRV_RTC_INT_TICK)
	{
	}
}

// start the low frequency clock
// usb_init() already starts this so as long as you do the rtc stuff after everything should work
static void lfclk_config(void)
{
	ret_code_t err_code = nrf_drv_clock_init();
	APP_ERROR_CHECK(err_code);

	nrf_drv_clock_lfclk_request(NULL);
}

// rtc init and configuration
static void rtc_config(void)
{
    uint32_t err_code;

    //Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = PRESCALER;
    err_code = nrf_drv_rtc_init(&rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    //nrf_drv_rtc_tick_enable(&rtc,true);

    //Set compare channel to START_TIME
    err_code = nrf_drv_rtc_cc_set(&rtc,0, START_TIME * RTC_FREQ,true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    //nrf_drv_rtc_enable(&rtc);
}
