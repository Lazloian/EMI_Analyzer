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

// Task for detecting input. Resumes and suspends tasks depending on the button pressed
void sensorTasks_input(void * pvParameter)
{
	int button; // stores the button that has been most recently pressed
	
	bool sweeping = false; // keeps track of state of measurements
	
	for (;;)
	{
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
		
		// delay the task
		vTaskDelay(MS_TO_TICK(INPUT_PERIOD));
	}
}

// Task for conducting sweeps at a certain time
void sensorTasks_sweep(void * pvParameter)
{
	TickType_t lastWakeTime; 								 // keeps track of the time the task woke up
	
	Sweep sweep;				// sweep to run
	uint32_t numSaved; // number of sweeps saved

  // this task starts suspended
  vTaskSuspend(NULL);
	
	for (;;)
	{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Executing Sweep");
				NRF_LOG_FLUSH();
#endif
		// get the current tick count
		lastWakeTime = xTaskGetTickCount();
		
		// turn on sweep LED
		gpioteManager_writePin(LED_SWEEP, 0);
		
		// get the sweep and numSaved from flash
		if (flashManager_checkConfig(&numSaved, &sweep))
		{
			// save a sweep to flash
			sensorFunctions_saveSweep(&sweep, &numSaved, false);
		}
		// turn off sweep LED
		gpioteManager_writePin(LED_SWEEP, 1);

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

// Task to blink the LED to indicate that the device is running
void sensorTasks_blink(void * pvParameter)
{
  // this task starts suspended
  vTaskSuspend(NULL);

	for (;;)
	{
#ifdef DEBUG_TASKS
				NRF_LOG_INFO("TASKS: Blink");
				NRF_LOG_FLUSH();
#endif
		// blink the rtc led
		gpioteManager_writePin(LED_RTC, 0);
		vTaskDelay(MS_TO_TICK(100));
		gpioteManager_writePin(LED_RTC, 1);
		
		// wait
		vTaskDelay(SEC_TO_TICK(BLINK_PERIOD));
	}
}

void sensorTasks_usb(void *pvParameter)
{
	Sweep sweep;						// the sweep stored in flash
	uint32_t numSaved; 		 // number of sweeps saved
	uint32_t pointer = 0; // the number of the sweep to be sent next
	uint8_t command[1];  // the command from usb
	
	for (;;)
	{
		// check if a usb device is plugged in
		while (usbManager_checkUSB())
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
	xret = xTaskCreate(sensorTasks_input, "inputTask", configMINIMAL_STACK_SIZE + 100, NULL, 2, &inputTask_handle);
  if (xret != pdPASS) return false;
  
	xret = xTaskCreate(sensorTasks_sweep, "sweepTask", configMINIMAL_STACK_SIZE + 340, NULL, 3, &sweepTask_handle);
  if (xret != pdPASS) return false;

	xret = xTaskCreate(sensorTasks_blink, "blinkTask", configMINIMAL_STACK_SIZE + 100, NULL, 1, &blinkTask_handle);
  if (xret != pdPASS) return false;
	
	xret = xTaskCreate(sensorTasks_usb, "usbTask", configMINIMAL_STACK_SIZE + 440, NULL, 2, &usbTask_handle);
  if (xret != pdPASS) return false;

  // start the sceduler
	vTaskStartScheduler();

	// this function should never return
  return false;
}
