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
#include "rtcManager.h"
#include "gpioteManager.h"
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
	sensorTasks_init();

  // set the sweep to default parameters
  sensorTasks_set_default(&sweep);
	
	// load the config files from flash
	flashManager_checkConfig(&numSaved, &sweep);
	
	// variable to know what sweeps have been sent
	uint32_t pointer = 0;
	
	uint8_t command[1]; // store the command from usb
	
  while (true)
  {
		if (rtcManager_checkCompare())
		{
			gpioteManager_writePin(LED_SWEEP, 0);
			nrf_delay_ms(100);
			gpioteManager_writePin(LED_SWEEP, 1);
			gpioteManager_writePin(LED_RTC, 1);
			gpioteManager_writePin(LED_AD5933, 1);
			
			sensorTasks_saveSweep(&sweep, &numSaved, false);
		}
			
		// check if usb data is available, if it is get the first command
		if (usbManager_readReady() && usbManager_getByte(command))
		{
			NRF_LOG_INFO("%x", command[0]);
			NRF_LOG_FLUSH();
			
			// read the config file
			// send the number of saved sweeps
			if (command[0] == 1)
			{
				sensorTasks_sendConfig(&sweep, &numSaved);
				
				// also reset the pointer here
				pointer = numSaved;
			}
			// execute a sweep and save to flash
			else if (command[0] == 2)
			{
				// this command was sent by usb, so usb in the function call is true
				sensorTasks_saveSweep(&sweep, &numSaved, true);
			}
			// execute a sweep and immedietly send it over usb, do not save to flash
			else if (command[0] == 3)
			{
		    sensorTasks_sweepAndSend(&sweep);
			}
			// send the pointer sweep over usb
			else if (command[0] == 4)
			{
				// if pointer is at file 0, set it at the most recent sweep saved
				if (pointer == 0) pointer = numSaved;
				
				// send the sweep at pointer
				sensorTasks_sendSweep(pointer);
				
				// move pointer down
				pointer--;
			}
    }
    // Sleep CPU only if there was no interrupt since last loop processing
    __WFE();
	}
}
