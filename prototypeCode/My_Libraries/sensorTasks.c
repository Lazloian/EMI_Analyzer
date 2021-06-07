/*
 *  sensorTasks.c
 *
 *  Contains all the tasks that will be run by FreeRTOS
 *
 *  Author: Henry Silva
 *
 */
 
#include "sensorTasks.h"

static TaskHandle_t inputTask_handle = NULL;
static TaskHandle_t sweepTask_handle = NULL;
static TaskHandle_t blinkTask_handle = NULL;
static TaskHandle_t usbTask_handle = NULL;
static TaskHandle_t BLETask_handle = NULL;

static bool bleReady = false;  	// is set to true when the BLE task is run
static bool init_done = false; // set to true when init is done
static bool unsent_sweeps = false; // is true when there are unsent sweeps over BLE

// Task for detecting input. Resumes and suspends tasks depending on the button pressed
void sensorTasks_input(void * pvParameter)
{
	int button; // stores the button that has been most recently pressed
	
	bool sweeping = false; // keeps track of state of measurements
	
	for (;;)
	{
    /*
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
	TickType_t lastWakeTime;	// keeps track of the time the task woke up
	Config config = {0};		 // saves config data
	
	// wait until init is finished
	while (!bleReady)
	{
		vTaskDelay(SEC_TO_TICK(1));
	}
	
	// init peripherals
  init_peripherals();
	
	init_done = true;

	// set the default sweep
	flashManager_checkConfig(&config);
	waitFDS();
  sensorFunctions_set_default(&config.sweep);
	flashManager_updateConfig(&config);
	waitFDS();

  if (config.num_sent < config.num_sweeps) unsent_sweeps = true;
	
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
	
	for (;;)
	{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Executing Sweep");
				NRF_LOG_FLUSH();
#endif
		// get the current tick count
		lastWakeTime = xTaskGetTickCount();
		
		// make sure nothing is happending on FDS
		waitFDS();
		
		// turn on sweep LED
		gpioteManager_writePin(LED_SWEEP, 2);
		
		// get the sweep and numSaved from flash
		if (flashManager_checkConfig(&config))
		{
			// update time in the metadata
			config.sweep.metadata.time = TICK_TO_SEC(xTaskGetTickCount());
			
#ifdef TESTING
			// FOR TESTING WITHOUT AD5933
			if (testFunctions_saveDummy(&config, false))
			{
				waitFDS();
				config.num_sweeps++;
				flashManager_updateConfig(&config);
				waitFDS();

        unsent_sweeps = true;
			}
#else
			// save the sweep with no usb update
			if (sensorFunctions_saveSweep(&config, false))
			{
        waitFDS();
				// update the number of saved sweeps
				config.num_sweeps += 1;
				flashManager_updateConfig(&config);
				waitFDS();

        unsent_sweeps = true;
			}
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
  bool success = false; // keeps track of the status of the current sweep send
  uint8_t command = 0; // to store the incoming BLE command
  uint32_t sent = 0;  // stores the number of data points sent
	unsigned char device_name[7]; // device name

  // pointers to store data to send
  uint32_t * freq;
  uint16_t * real;
  uint16_t * imag;
  MetaData meta;
	
	// wait till init is done
	while (!init_done)
	{
		vTaskDelay(MS_TO_TICK(500));
	}
	
	// check the config for a device ID
	waitFDS();
	flashManager_checkConfig(&config);

	if (config.device_id[0] == 0)
	{
		uint8_t bytes_avail = 0; // buffer to store the available random bytes
		
		// check if there are enough random bytes. If not, wait for them
		sd_rand_application_bytes_available_get(&bytes_avail);
		while (bytes_avail < sizeof(config.device_id))
		{
			vTaskDelay(500);
			sd_rand_application_bytes_available_get(&bytes_avail);
		}
		// assign random device ID and adjust values
		sd_rand_application_vector_get(config.device_id, sizeof(config.device_id));
		
		for (int i = 0; i < sizeof(config.device_id); i++)
		{
			config.device_id[i] = (int) ((((float) config.device_id[i]) / UINT8_MAX) * (90 - 65)) + 65;
		}
	}
	
	sprintf((char *) device_name, "EMI_%c%c%c", config.device_id[0], config.device_id[1], config.device_id[2]);
	
	bleManager_changeName(device_name, sizeof(device_name));
	
	flashManager_updateConfig(&config);
	waitFDS();
	
#ifdef DEBUG_TASKS
	NRF_LOG_INFO("TASKS: BLE Device ID set to %c%c%c", config.device_id[0], config.device_id[1], config.device_id[2]);
	NRF_LOG_FLUSH();
#endif
	
	for (;;)
	{
		// if the BLE connection is alive, check if there are sweeps to send
    if (bleManager_conn_status() == BLE_CON_ALIVE)
    {
#ifdef DEBUG_TASKS
			NRF_LOG_INFO("TASKS: BLE Connected");
			NRF_LOG_FLUSH();
#endif
			
			// make sure nothing is happening on FDS
			waitFDS();
			
      // load config to check for sweep to send
      flashManager_checkConfig(&config);

      // if there is a sweep to send then send it over BLE
      if (config.num_sent < config.num_sweeps)
      {
        // allocate memory and get sweep
        freq = nrf_malloc(MAX_FREQ_SIZE);
        real = nrf_malloc(MAX_IMP_SIZE);
        imag = nrf_malloc(MAX_IMP_SIZE);
      
        flashManager_getSweep(freq, real, imag, &meta, config.num_sent + 1);

        // set sent to 0 and success to false
        sent = 0;
        success = false;
				
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: BLE transfer started");
				NRF_LOG_FLUSH();
#endif
				
        // while connection if alive run this task with a short delay
        while (bleManager_conn_status() == BLE_CON_ALIVE)
        {
          // check for valid command
          switch (bleManager_get_command())
          {
            // do nothing if command is NULL
            case NULL:
              break;
            // command '0' requests metadata for the sweep
            case 48:
              bleManager_send_meta(&meta);
              break;
            // command '1' is a request for sweep data
            case 49:
              sent = bleManager_sendSweep(&meta, freq, real, imag, sent);
              // if sendSweep returns 0 then the whole sweep has been sent
              if (sent == 0) success = true;
              break;
						// command '2' is a request to delete all sweeps on flash
						case 50:
							delete_sweeps(&config, false);
							break;
          }
          vTaskDelay(MS_TO_TICK(200));
        }

        // free the memory
        nrf_free(freq);
        nrf_free(real);
        nrf_free(imag);

        // check if BLE transfer successful
        if (success)
        {
          // udpate the config
          config.num_sent++;
          flashManager_updateConfig(&config);

					waitFDS();

          if (config.num_sent == config.num_sweeps) unsent_sweeps = false;
#ifdef DEBUG_BLE
          NRF_LOG_INFO("TASKS: BLE transfer complete");
          NRF_LOG_FLUSH();
#endif
        }
        else
        {
#ifdef DEBUG_BLE
          NRF_LOG_INFO("TASKS: BLE transfer failed");
          NRF_LOG_FLUSH();
#endif
        }
				// if there are still sweeps to send, advertise
				if (unsent_sweeps && !bleManager_adv_status())
				{
					bleManager_adv_begin();
				}
      }
    }
		vTaskDelay(SEC_TO_TICK(BLE_PERIOD));
	}
}

// Task to blink the LED to indicate that the device is running
// Every time the LED blinks it also advertises over BLE
void sensorTasks_blink(void * pvParameter)
{
  uint16_t counter = 0; // keeps to track of when the advertise
  Config config;
	
	while (!init_done)
	{
		vTaskDelay(MS_TO_TICK(500));
	}
	
	// init finished, turn LED 1 off
	gpioteManager_writePin(LED_1, 2);

	for (;;)
	{
#ifdef DEBUG_TASKS
		NRF_LOG_INFO("TASKS: Blink at %d", xTaskGetTickCount());
		NRF_LOG_FLUSH();
#endif
		// blink the rtc led
		gpioteManager_writePin(LED_BLINK, 2);
		vTaskDelay(MS_TO_TICK(100));
		gpioteManager_writePin(LED_BLINK, 2);
		
		// check if enough time has passed to advertise and that there is no current BLE connection and (most importantly) that there are new sweeps to send
		if (unsent_sweeps && bleManager_conn_status() == BLE_CON_DEAD && (counter++ * BLINK_PERIOD) >= ADV_PERIOD)
		{
      // advertise and set counter to 0
			bleManager_adv_begin();
      counter = 0;

      // blink again
			vTaskDelay(MS_TO_TICK(100));
      gpioteManager_writePin(LED_BLINK, 2);
      vTaskDelay(MS_TO_TICK(100));
      gpioteManager_writePin(LED_BLINK, 2);
		}
		
		// wait
		vTaskDelay(SEC_TO_TICK(BLINK_PERIOD));
	}
}

void sensorTasks_usb(void *pvParameter)
{
	Config config;
	uint32_t pointer = 0; // the number of the sweep to be sent next
	uint8_t command[1];  // the command from usb
	
	// wait for BLE to be ready
	while (!bleReady)
	{
		vTaskDelay(MS_TO_TICK(500));
	}
	
	// init usb
	usbManager_init();
	
	for (;;)
	{
		// check if a usb device is plugged in
		while (usbManager_checkUSB())
		{
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
					if (testFunctions_saveDummy(&config, true))
					{
						waitFDS();
						config.num_sweeps++;
						flashManager_updateConfig(&config);
					}
					
#else
					// this command was sent by usb, so usb in the function call is true
					sensorFunctions_saveSweep(&config, true);
#endif
					unsent_sweeps = true;
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
					// this no longer works with softdevice, must do manually
					//sensorFunctions_deleteSweeps(&config, true);
					
					delete_sweeps(&config, true);
				}
			}
			waitFDS();
		}
		
		// delay the task
		vTaskDelay(MS_TO_TICK(USB_PERIOD));
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

	xret = xTaskCreate(sensorTasks_blink, "blinkTask", configMINIMAL_STACK_SIZE + 140, NULL, 1, &blinkTask_handle);
  if (xret != pdPASS) return false;
	
	xret = xTaskCreate(sensorTasks_usb, "usbTask", configMINIMAL_STACK_SIZE + 240, NULL, 2, &usbTask_handle);
  if (xret != pdPASS) return false;
	
	nrf_sdh_freertos_init(readyBLE, NULL);
	
	xret = xTaskCreate(sensorTasks_BLE, "BLETask", configMINIMAL_STACK_SIZE + 340, NULL, 2, &BLETask_handle);
  if (xret != pdPASS) return false;

  // start the sceduler
	vTaskStartScheduler();

	// this function should never return
  return false;
}

// --- Helper Functions ---
// inits all the peripherals that must init after BLE
static void init_peripherals(void)
{
	// Delay until BLE is ready
	while (!bleReady)
	{
		vTaskDelay(MS_TO_TICK(500));
	}
	
	// init flash
	flashManager_init();
	
	// wait till flash init is done
	waitFDS();
	
	return;
}
// checks if FDS is done processing commands and waits if it isn't
static void waitFDS(void)
{
	while (!flashManager_checkComplete()) vTaskDelay(MS_TO_TICK(500));
	
	return;
}
// function that the BLE task calls before executing. This lets BLE init before other tasks start
static void readyBLE(void * parameter)
{
	bleReady = true;
	return;
}

// deletes all sweeps on flash
static bool delete_sweeps(Config * config, bool usb)
{
	bool ret = true; // keeps track of flashManager status
	uint8_t queue = 0; // keeps track of the queued commands
	
	// make sure nothing is being processed in FDS
	waitFDS();
	
	// delete files and make sure the queue does not overflow
	while (config->num_sweeps > 0 && ret)
	{
		while (queue < FDS_OP_QUEUE_SIZE && config->num_sweeps > 0 && ret)
		{
			ret = flashManager_deleteSweep(config->num_sweeps--);
			config->num_deleted++;
			queue++;
		}
		waitFDS();
		queue = 0;
	}
	// update config if success
	if (ret)
	{
		flashManager_updateConfig(config);
	}
	
	if (usb)
	{
		uint8_t buff[1];
		if (ret) 
		{
			buff[0] = 5;
		}
		else
		{
			buff[0] = 6;
		}
		usbManager_writeBytes(buff, 1);
	}
	
	// all sweeps just got deleted so run GC
	if (ret) flashManager_collectGarbage();
	waitFDS();
	
	return ret;
}

// overflow hook for freertos. If enabled, it will display the task that overflowed to the log
#if (configCHECK_FOR_STACK_OVERFLOW == 1 || configCHECK_FOR_STACK_OVERFLOW == 2)
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
#ifdef DEBUG_TASKS
	NRF_LOG_INFO("TASKS: %s stack overflow", pcTaskName);
	NRF_LOG_FLUSH();
#endif
	return;
}
#endif
