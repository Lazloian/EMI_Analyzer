/*
 * This program takes commands from a python script and sends back impedance data
 * over usb
 *
 */

// standard includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_power.h"
#include "app_error.h"
#include "app_util.h"
#include "app_timer.h"

#include "nrf_delay.h"
#include "boards.h"
#include "app_util_platform.h"

#include "mem_manager.h"

#include "fds.h"

#include "nrf_drv_rtc.h"

// logging includes
#ifdef DEBUG_LOG
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

// my modules include
#include "twiManager.h"
#include "AD5933.h"
#include "flashManager.h"
#include "usbManager.h"

// --- User Defines ---

#define BUTTON_START (BSP_BUTTON_0) // Button 1 starts rtc
#define BUTTON_STOP  (BSP_BUTTON_1) // Button 2 stops rtc

#define LED_RTC    (BSP_LED_0) // LED to signal if RTC is on
#define LED_USB    (BSP_LED_1) // LED to signal if USB if connected
#define LED_SWEEP  (BSP_LED_2) // LED to signal if sweep is being done
#define LED_AD5933 (BSP_LED_3) // LED to singal if the AD5933 is connected

bool recieveSweep(Sweep * sweep);
void set_default(Sweep * sweep);
bool saveSweep(void);

// variable to store the number of saved sweeps
static uint32_t numSweeps = 0;

// create a new sweep
static Sweep sweep = {0};

// --- RTC Defines ---
#define RTC_FREQ 8 																 // RTC frequency in Hz
#define PRESCALER RTC_FREQ_TO_PRESCALER(RTC_FREQ) // prescaler for RTC_FREQ
#define COMPARE_TIME 1800												 // number of seconds between compare events

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
static void rtc_handler(nrf_drv_rtc_int_type_t int_typestatic);
void lfclk_config(void);
static void rtc_config(void);

// to keep track of the number of compare events triggered
static uint32_t compares = 1;

// --- GPIOTE Defines ---
void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void gpiote_init(void);

// Main function
int main(void)
{
  bool i2c_stats;						// stores if i2c success

  // init Log
#ifdef DEBUG_LOG
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  NRF_LOG_INFO("Wireless Sensor Started");
  NRF_LOG_FLUSH();
#endif

  // init twi
  twiManager_init();

  // init USB
  usbManager_init();
	
	// init flashManager
	flashManager_init();
	
	// init memory manager
	nrf_mem_init();
	
	// init rtc (must be done after init_usb() due to the low frequency clock being needed)
	rtc_config();
	
	// init LEDS
  // bsp_board_init(BSP_INIT_LEDS);
	
	gpiote_init();

  // reset the AD5933
  i2c_stats = AD5933_SetControl(NO_OPERATION, RANGE1, GAIN1, INTERN_CLOCK, 1);
	
	// create a new sweep
	Sweep oldSweep = {0};

  // set the sweep to default parameters
  set_default(&sweep);
	
	// variable to know what sweeps have been sent
	uint32_t pointer = 0;
	
	// load the config files from flash
	flashManager_checkConfig(&numSweeps, &oldSweep);

  if (i2c_stats) {nrf_drv_gpiote_out_clear(LED_AD5933);}
#ifdef DEBUG_LOG
  else {NRF_LOG_INFO("AD5933 Connection Failed");}
  NRF_LOG_FLUSH();
#endif
	
	uint8_t command[1]; // store the command from usb
	
  while (true)
  {
		// check if usb data is available, if it is get the first command
		if (usbManager_readReady() && usbManager_getByte(command))
		{
			NRF_LOG_INFO("%x", command[0]);
			NRF_LOG_FLUSH();
			
			// read the config file
			// send the number of saved sweeps
			if (command[0] == 1)
			{
				flashManager_checkConfig(&numSweeps, &oldSweep);
				usbManager_writeBytes(&numSweeps, sizeof(numSweeps));
				
				// also reset the pointer here
				pointer = numSweeps;
			}
			// execute a sweep and save to flash
			else if (command[0] == 2)
			{
				// allocate memory for sweep data
				uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
				uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
				uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
				
				uint8_t buff[1]; // to save the result

				if (AD5933_Sweep(&sweep, freq, real, imag))
				{
					if (flashManager_saveSweep(freq, real, imag, &sweep.metadata, numSweeps + 1))
					{
						numSweeps += 1;
						flashManager_updateNumSweeps(&numSweeps);
#ifdef DEBUG_LOG
						NRF_LOG_INFO("Sweep %d saved", numSweeps);
						NRF_LOG_FLUSH();
#endif
						
						// send sweep success over usb
						buff[0] = 2;
					}
					else
					{
						buff[0] = 1;
					}
				}
				else
				{
					buff[0] = 1;
				}
				// send result
				usbManager_writeBytes(buff, 1);
				
				// free the memory
				nrf_free(freq);
				nrf_free(real);
				nrf_free(imag);
			}
			// execute a sweep and immedietly send it over usb, do not save to flash
			else if (command[0] == 3)
			{
				// allocate memory for sweeps
				uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
				uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
				uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
				
				// execute the sweep
				if (AD5933_Sweep(&sweep, freq, real, imag))
				{
#ifdef DEBUG_LOG
					NRF_LOG_INFO("Sending Sweep");
#endif
					if (usbManager_sendSweep(freq, real, imag, &sweep.metadata))
					{
#ifdef DEBUG_LOG
						NRF_LOG_INFO("Sweep Send Success");
#endif
					}
					else
					{
#ifdef DEBUG_LOG
						NRF_LOG_INFO("Sweep Send Fail");
#endif
					}
#ifdef DEBUG_LOG
					NRF_LOG_FLUSH();
#endif
				}
				
				// free the memory
				nrf_free(freq);
				nrf_free(real);
				nrf_free(imag);
			}
			// send the pointer sweep over usb
			else if (command[0] == 4)
			{
				// allocate memory for sweeps
				uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
				uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
				uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
				
				// if pointer is at file 0, set it at the most recent sweep saved
				if (pointer == 0) pointer = numSweeps;
					
				// get the sweep data from flash
				if (flashManager_getSweep(freq, real, imag, &sweep.metadata, pointer))
				{
					if (usbManager_sendSweep(freq, real, imag, &sweep.metadata))
					{
#ifdef DEBUG_LOG
						NRF_LOG_INFO("Sweep send from flash success");
						NRF_LOG_FLUSH();
#endif
					}
					else
					{
#ifdef DEBUG_LOG
						NRF_LOG_INFO("Sweep send fail");
						NRF_LOG_FLUSH();
#endif
					}
				}
				
				// free memory
				nrf_free(freq);
				nrf_free(real);
				nrf_free(imag);
				
				// move pointer down
				pointer--;
			}
    }
    // Sleep CPU only if there was no interrupt since last loop processing
    __WFE();
	}
}


bool saveSweep(void)
{
	// allocate memory for sweep data
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
	uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	
	bool res = false; // saves if sweep success

	if (AD5933_Sweep(&sweep, freq, real, imag))
	{
		if (flashManager_saveSweep(freq, real, imag, &sweep.metadata, numSweeps + 1))
		{
			numSweeps += 1;
			flashManager_updateNumSweeps(&numSweeps);
#ifdef DEBUG_LOG
			NRF_LOG_INFO("Sweep %d saved", numSweeps);
			NRF_LOG_FLUSH();
#endif
			res = true;
		}
		else
		{
#ifdef DEBUG_LOG
			NRF_LOG_INFO("Sweep save fail");
			NRF_LOG_FLUSH();
#endif
		}
	}
	else
	{
#ifdef DEBUG_LOG
		NRF_LOG_INFO("Sweep save fail");
		NRF_LOG_FLUSH();
#endif
	}
	
	// free the memory
	nrf_free(freq);
	nrf_free(real);
	nrf_free(imag);
	
	return res;
}

// recieves usb data for a sweep parameter over usb
bool recieveSweep(Sweep * sweep)
{
  uint8_t params[16];

  usbManager_readBytes(params, 16);

  return true;
}

void set_default(Sweep * sweep)
{
  // set the default sweep parameters
  sweep->start 							= 1000;
  sweep->delta 							= 100;
  sweep->steps 							= 490;
  sweep->cycles 						= 511;
  sweep->cyclesMultiplier 	= TIMES4;
  sweep->range 							= RANGE1;
  sweep->clockSource 				= INTERN_CLOCK;
  sweep->clockFrequency 		= CLK_FREQ;
  sweep->gain 							= GAIN1;
	sweep->metadata.numPoints = sweep->steps + 1;
	sweep->metadata.temp			= 100;
	sweep->metadata.time			= 30;

  return;
}

// --- GPIOTE Functions ---

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	// check which button
	if (pin == BUTTON_START && action == GPIOTE_CONFIG_POLARITY_HiToLo)
	{
		nrf_drv_rtc_enable(&rtc);
		nrf_drv_gpiote_out_clear(LED_RTC);
	}
	else if (pin == BUTTON_STOP && action == GPIOTE_CONFIG_POLARITY_HiToLo)
	{
		nrf_drv_rtc_disable(&rtc);
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

// --- RTC Functions ---

// Handler for RTC interrupts like the tick and compare
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
	// check if compare or tick event
	if (int_type == NRF_DRV_RTC_INT_COMPARE0)
	{
		// what to do when the time to compare to has been triggered
		nrf_drv_gpiote_out_clear(LED_SWEEP);
		nrf_delay_ms(100);
		nrf_drv_gpiote_out_set(LED_SWEEP);
		saveSweep();
		nrf_drv_gpiote_out_set(LED_SWEEP);
		nrf_drv_gpiote_out_set(LED_RTC);
		nrf_drv_gpiote_out_set(LED_AD5933);
		nrf_drv_rtc_cc_set(&rtc, 0, compares * (COMPARE_TIME * RTC_FREQ), true);
	  compares++;
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

    //Set compare channel to trigger interrupt after 15 seconds
    err_code = nrf_drv_rtc_cc_set(&rtc,0, 15 * RTC_FREQ,true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    //nrf_drv_rtc_enable(&rtc);
}
