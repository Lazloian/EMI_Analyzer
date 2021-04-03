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

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

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
//#include "rtcManager.h"
#include "gpioteManager.h"
#include "sensorFunctions.h"
#include "sensorTasks.h"

// variable to store the number of saved sweeps
static uint32_t numSaved = 0;

// create a new sweep
static Sweep sweep = {0};

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
	
	// set the sweep to default parameters
  sensorFunctions_set_default(&sweep);
	
	// load the config files from flash
	flashManager_checkConfig(&numSaved, &sweep);
	
	// set the sweep to default parameters
  sensorFunctions_set_default(&sweep);
	
	// set the sweep to the current default
	flashManager_updateSavedSweep(&sweep);
	
	// Go into low power mode
	__WFE();
	
	// init the tasks
	sensorTasks_init(); // this function should never return
	
	// SHOULD NEVER RUN THE BELOW CODE
	// variable to know what sweeps have been sent
	uint32_t pointer = 0;
	
	uint8_t command[1]; // store the command from usb
	
  while (true)
  {
		if (true)
		{
#ifdef DEBUG_LOG
			NRF_LOG_INFO("I should not be here");
			NRF_LOG_FLUSH();
#endif
			nrf_delay_ms(1000);
		}
		else
		{
			
			// check if usb data is available, if it is get the first command
			if (usbManager_readReady() && usbManager_getByte(command))
			{
				// update the sweep and numSaved
				flashManager_checkConfig(&numSaved, &sweep);
				
				// read the config file
				// send the number of saved sweeps
				if (command[0] == 1)
				{
					sensorFunctions_sendConfig(&sweep, &numSaved);
					
					// also reset the pointer here
					pointer = numSaved;
				}
				// execute a sweep and save to flash
				else if (command[0] == 2)
				{
					// this command was sent by usb, so usb in the function call is true
					sensorFunctions_saveSweep(&sweep, &numSaved, true);
				}
				// execute a sweep and immedietly send it over usb, do not save to flash
				else if (command[0] == 3)
				{
					sensorFunctions_sweepAndSend(&sweep);
				}
				// send the pointer sweep over usb
				else if (command[0] == 4)
				{
					// if pointer is at file 0, set it at the most recent sweep saved
					if (pointer == 0) pointer = numSaved;
					
					// send the sweep at pointer
					sensorFunctions_sendSweep(pointer);
					
					// move pointer down
					pointer--;
				}
			}
			// Sleep CPU only if there was no interrupt since last loop processing
			__WFE();
		}
	}
}
