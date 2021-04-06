/*
 * Wireless Sensor main file
 */

// standard includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "app_error.h"
#include "app_util.h"

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
#include "gpioteManager.h"
#include "sensorFunctions.h"
#include "sensorTasks.h"

// variable to store the number of saved sweeps
static uint32_t numSaved = 0;

// create a new sweep
static Sweep sweep = {0};

// stores the number of sweeps deleted
static uint16_t numDeleted = 0;

// Main function
int main(void)
{
	// init logging if enabled
#ifdef DEBUG_LOG
	 APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
  NRF_LOG_DEFAULT_BACKENDS_INIT();
#endif
	
	// init peripherals for tasks
	sensorFunctions_init();
	
	// turn on LED to indicate init
	gpioteManager_writePin(RAK_LED_1, 1);
	
	// set the sweep to default parameters
  sensorFunctions_set_default(&sweep);
	
	// load the config files from flash
	flashManager_checkConfig(&numSaved, &sweep, &numDeleted);
	
	// set the sweep to default parameters
  sensorFunctions_set_default(&sweep);
	
	// set the sweep to the current default
	flashManager_updateSavedSweep(&sweep);
	
	// check if the number of sweeps that have been deleted is greater than FM_MAX_DELETE
	if (numDeleted >= FM_MAX_DELETE)
	{
		if (flashManager_collectGarbage())
		{
			numDeleted = 0;
			flashManager_updateNumDeleted(&numDeleted);
		}
	}
	
	// turn off led to indicate init complete
	gpioteManager_writePin(RAK_LED_1, 0);
	
	// init the tasks
	sensorTasks_init(); // this function should never return
	
	// SHOULD NEVER RUN THE BELOW CODE
	
  while (true)
  {
#ifdef DEBUG_LOG
		NRF_LOG_INFO("I should not be here. FreeRTOS init has failed");
		NRF_LOG_FLUSH();
#endif
		nrf_delay_ms(1000);
	}
}
