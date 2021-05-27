/*
 *  sensorTasks.c
 *
 *  Contains all the tasks that will be run by FreeRTOS
 *
 *  Author: Henry Silva
 *
 */
 
#include "sensorTasks.h"

static TaskHandle_t inputTask_handle;
static TaskHandle_t sweepTask_handle;
static TaskHandle_t blinkTask_handle;
static TaskHandle_t usbTask_handle;
static TaskHandle_t BLETask_handle;
static TaskHandle_t advTask_handle;

static bool bleReady = false; // is set to true when the BLE task is run

// Task for detecting input. Resumes and suspends tasks depending on the button pressed
void sensorTasks_input(void * pvParameter)
{
	int button; // stores the button that has been most recently pressed
	
	bool sweeping = false; // keeps track of state of measurements
	
	for (;;)
	{
		/* No checking buttong presses on the RAK
		// check if a button has been pressed
		if (gpioteManager_getEvent(&button))
		{
			// start the sweep task
			if (button == BUTTON_START && !sweeping)
			{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Starting sweeps");
				NRF_LOG_FLUSH();
#endif
				sweeping = true;

        // resume the blink task
        vTaskResume(blinkTask_handle);
				
				// resume the sweep task
        vTaskResume(sweepTask_handle);
			}
			// suspend the sweep task
			else if (button == BUTTON_STOP && sweeping)
			{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Stopping sweeps");
				NRF_LOG_FLUSH();
#endif	
				sweeping = false;

        // suspend the sweep task
        vTaskSuspend(sweepTask_handle);

        // suspend the blink task
        vTaskSuspend(blinkTask_handle);
			}
		}
		*/
		
		// delay the task
		vTaskDelay(MS_TO_TICK(INPUT_PERIOD));
	}
}

// Task for conducting sweeps at a certain time
void sensorTasks_sweep(void * pvParameter)
{
	TickType_t lastWakeTime; 								 // keeps track of the time the task woke up
	
	Config config = {0};	// saves config data

  // Delay the task until BLE is ready
	while (!bleReady)
	{
		vTaskDelay(MS_TO_TICK(500));
	}
	
	// init flash (trying it here)
	flashManager_init();
	
	// set the default sweep
	flashManager_checkConfig(&config);
	waitFDS();
  sensorFunctions_set_default(&config.sweep);
	flashManager_updateConfig(&config);
	waitFDS();
	
	// check if garbage collection must be run
	if (config.num_deleted >= FM_MAX_DELETE)
	{
		if (flashManager_collectGarbage())
		{
			waitFDS();
			config.num_deleted = 0;
			flashManager_updateConfig(&config);
		}
	}
	
	// turn off LED to indicate flash init success
	gpioteManager_writePin(RAK_LED_1, 0);
	
	for (;;)
	{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Executing Sweep");
				NRF_LOG_FLUSH();
#endif
		// get the current tick count
		lastWakeTime = xTaskGetTickCount();
		
		// turn on sweep LED
		gpioteManager_writePin(LED_SWEEP, 2);
		
		// this protects against overflowing the FDS queue
		waitFDS();
		
		// get the sweep and numSaved from flash
		if (flashManager_checkConfig(&config))
		{
			// update time in the metadata
			config.sweep.metadata.time = TICK_TO_SEC(xTaskGetTickCount());
			
#ifdef TESTING
			// FOR TESTING WITHOUT AD5933
			testFunctions_saveDummy(&config, true);
			waitFDS();
			config.num_sweeps++;
			flashManager_updateConfig(&config);
#else
			// save the sweep with no usb update
			sensorFunctions_saveSweep(&config, false);
			waitFDS();
			// update the number of saved sweeps
			config.num_sweeps += 1;
			flashManager_updateConfig(&config);
#endif
		}
		// turn off sweep LED
		gpioteManager_writePin(LED_SWEEP, 2);

#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Sweep Executed");
				NRF_LOG_FLUSH();
#endif
		
		// delay until a period of time has passed from the start of the sweep
		// vTaskDelayUntil is used because a sweep may take up to a minute to complete
		// This time must be accounted for when determining the time to the next sweep
		vTaskDelayUntil(&lastWakeTime, SEC_TO_TICK(SWEEP_PERIOD));
	}
	
}

// Task for sending sweep data over BLE
void sensorTasks_BLE(void * pvParameter)
{
	Config config; // config to store sensor config
  uint16_t delay = SEC_TO_TICK(BLE_PERIOD); // delay of the BLE task
  bool sending = false; // keeps track of a current ble transaction

  // pointers to store data to send
  uint32_t * freq;
  uint16_t * real;
  uint16_t * imag;
  MetaData meta;
	
	// Delay the task to check usb
	vTaskDelay(SEC_TO_TICK(START_DELAY));
	
	for (;;)
	{
		// check if BLE connection is active
    if (ble_check_connection() == BLE_CON_ALIVE)
    {
      if (!sending)
      {
        // read the config
        flashManager_checkConfig(&config);
        
        // check if the number of sweeps in flash is equal to the number of sweeps sent over BLE
        if (config.num_sent < config.num_sweeps)
        {
#ifdef DEBUG_TASKS
					NRF_LOG_INFO("Tasks: Sending Sweep Over BLE");
					NRF_LOG_FLUSH();
#endif
          // allocate memory, get the next unsent sweep, and stage it
          freq = nrf_malloc(MAX_FREQ_SIZE);
          real = nrf_malloc(MAX_IMP_SIZE);
          imag = nrf_malloc(MAX_IMP_SIZE);
				
          flashManager_getSweep(freq, real, imag, &meta, config.num_sent + 1);
					
          ble_stage_sweep(freq, real, imag, &meta);

          // start data transfer
          ble_command_handler();

          delay = MS_TO_TICK(150);
          sending = true;
        }
      }
      else
      {
        // check connection and run the command handler
        if (ble_command_handler() == BLE_TRANSFER_COMPLETE)
        {
          // unstage sweep, free memory, and set sending to false
          ble_unstage_sweep();
          
          nrf_free(freq);
          nrf_free(real);
          nrf_free(imag);

          sending = false;
          delay = SEC_TO_TICK(BLE_PERIOD);

          // update the config
          config.num_sent++;
          flashManager_updateConfig(&config);
					waitFDS();
#ifdef DEBUG_BLE
          NRF_LOG_INFO("TASKS: BLE transfer complete");
          NRF_LOG_FLUSH();
#endif
        } 
      }
		}
    else
    {
      if (sending)
      {
        // unstage sweep, free memory, and set sending to false
        ble_unstage_sweep();
        
        nrf_free(freq);
        nrf_free(real);
        nrf_free(imag);

        sending = false;
        delay = SEC_TO_TICK(BLE_PERIOD);
#ifdef DEBUG_BLE
        NRF_LOG_INFO("TASKS: BLE transfer failed");
        NRF_LOG_FLUSH();
#endif
      }
    }
		vTaskDelay(delay);
	}
}

// Task to blink the LED to indicate that the device is running
void sensorTasks_blink(void * pvParameter)
{
  // Delay the task to check usb
	vTaskDelay(SEC_TO_TICK(START_DELAY));

	for (;;)
	{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Blink");
				NRF_LOG_FLUSH();
#endif
		// blink the rtc led
		gpioteManager_writePin(LED_BLINK, 2);
		vTaskDelay(MS_TO_TICK(100));
		gpioteManager_writePin(LED_BLINK, 2);
		
		// wait
		vTaskDelay(SEC_TO_TICK(BLINK_PERIOD));
	}
}

// This task turns on BLE advertising at a set time interval
void sensorTasks_adv(void * pvParameter)
{
  // nothing needs to be done before the task enters its loop
  
  for (;;)
  {
    // if advertising is not running, enable it
    if (!ble_is_advertising())
    {
      ble_advertise_begin();
    }

    // delay the task
    vTaskDelay(SEC_TO_TICK(ADV_PERIOD));
  }
}

void sensorTasks_usb(void *pvParameter)
{
	Config config;
	uint32_t pointer = 0; // the number of the sweep to be sent next
	uint8_t command[1];  // the command from usb
	
	for (;;)
	{
		// check if a usb device is plugged in
		while (usbManager_checkUSB())
		{
			// suspend the sweep task
			vTaskSuspend(sweepTask_handle);
			
			// check if usb data is available, if it is get the first command
			if (usbManager_readReady() && usbManager_getByte(command))
			{
				// update the sweep and numSaved
				flashManager_checkConfig(&config);
				
				// read the config file
				// send the number of saved sweeps
				if (command[0] == 1)
				{
					sensorFunctions_sendConfig(&config);
					
					// also reset the pointer here
					pointer = config.num_sweeps;
				}
				// execute a sweep and save to flash
				else if (command[0] == 2)
				{
#ifdef TESTING
					// FOR TESTING WITHOUT AD5933
					testFunctions_saveDummy(&config, true);
#else
					// this command was sent by usb, so usb in the function call is true
					sensorFunctions_saveSweep(&config, true);
#endif
				}
				// execute a sweep and immedietly send it over usb, do not save to flash
				else if (command[0] == 3)
				{
					sensorFunctions_sweepAndSend(&config.sweep);
				}
				// send the pointer sweep over usb
				else if (command[0] == 4)
				{
					// if pointer is at file 0, set it at the most recent sweep saved
					if (pointer == 0) pointer = config.num_sweeps;
					
					// send the sweep at pointer
					sensorFunctions_sendSweep(pointer);
					
					// move pointer down
					pointer--;
				}
				else if (command[0] == 5)
				{
					sensorFunctions_deleteSweeps(&config, true);
				}
			}
		}
		
		// delay the task
		vTaskDelay(MS_TO_TICK(USB_PERIOD));
		
		// resume the sweep task
		vTaskResume(sweepTask_handle);
	}
}

// This function is the callback function for the idle task. It will be called when no other task is running
// The function just puts the nrf into low power mode
void vApplicationIdleHook(void)
{
	// low power mode
	__WFE();
	
	return;
}

// Creates the necessary tasks and starts the FreeRTOS scheduler
// Return value:
//  false if error creating tasks
//  true if task creation success
bool sensorTasks_init(void)
{
	BaseType_t xret; // stores result of task creation

  // create the tasks
	
	// input task currently disabled for RAK due to it not having buttons
	// xret = xTaskCreate(sensorTasks_input, "inputTask", configMINIMAL_STACK_SIZE + 100, NULL, 2, &inputTask_handle);
  // if (xret != pdPASS) return false;
  
	xret = xTaskCreate(sensorTasks_sweep, "sweepTask", configMINIMAL_STACK_SIZE + 340, NULL, 3, &sweepTask_handle);
  if (xret != pdPASS) return false;

	xret = xTaskCreate(sensorTasks_blink, "blinkTask", configMINIMAL_STACK_SIZE + 300, NULL, 1, &blinkTask_handle);
  if (xret != pdPASS) return false;
	
	//xret = xTaskCreate(sensorTasks_usb, "usbTask", configMINIMAL_STACK_SIZE + 640, NULL, 2, &usbTask_handle);
  //if (xret != pdPASS) return false;
	
	nrf_sdh_freertos_init(readyBLE, NULL);
	
	xret = xTaskCreate(sensorTasks_BLE, "BLETask", configMINIMAL_STACK_SIZE + 340, NULL, 2, &BLETask_handle);
  if (xret != pdPASS) return false;

	xret = xTaskCreate(sensorTasks_adv, "advTask", configMINIMAL_STACK_SIZE, NULL, 1, &advTask_handle);
  if (xret != pdPASS) return false;

  // start the sceduler
	vTaskStartScheduler();

	// this function should never return
  return false;
}

// --- Helper Function ---
static void waitFDS(void)
{
	while (!flashManager_checkComplete()) vTaskDelay(MS_TO_TICK(250));
	
	return;
}

static void readyBLE(void * parameter)
{
	bleReady = true;
	return;
}
