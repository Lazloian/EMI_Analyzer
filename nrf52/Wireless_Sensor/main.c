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

// stores the sensor config
//static Config config;

// Main function
int main(void)
{
	// FOR DK: DEV_KIT TESTING DEBUG_BLE DEBUG_FUNCTIONS DEBUG DEBUG_LOG DEBUG_USB DEBUG_TASKS DEBUG_FLASH
	// init logging if enabled
#ifdef DEBUG_LOG
	APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
  NRF_LOG_DEFAULT_BACKENDS_INIT();
#endif
	
	// init peripherals and BLE
	sensorFunctions_init();

	// turn on LED to indicate init
	gpioteManager_writePin(LED_1, 2);
	
	// init the tasks
	sensorTasks_init(); // this function should never return

	// SHOULD NEVER RUN THE BELOW CODE
	
  while (true)
  {
#ifdef DEBUG_LOG
		NRF_LOG_INFO("I should not be here. FreeRTOS init has failed");
		NRF_LOG_FLUSH();
#endif
		nrf_delay_ms(2500);
	}
}
