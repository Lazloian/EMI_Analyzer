/*
 *  sensorFunctions.c
 *
 *  Contains all the tasks the sensor will execute
 *
 *  Author: Henry Silva
 *
 */
 
 #include "sensorFunctions.h"
 
// Checks and sends the number of saved sweeps over usb
// Arguments: 
//	* sweep    : The sweep to execute
//	* numSaved : The number of sweeps currently saved
//  * numSaved : The number of sweeps currently deleted
// Return value:
//  false if error
//  true  if success
 bool sensorFunctions_sendConfig(Config * config)
 {
	// send the number of saved sweeps
	if (!usbManager_writeBytes(&config->num_sweeps, sizeof(config->num_sweeps))) return false;
	 
	// success
	return true;
 }
 
// Gets a sweep from flash and sends it over usb
// Arguments: 
 //	sweepNum : The number of file that the sweep is located in
// Return value:
//  false if error
//  true  if success
 bool sensorFunctions_sendSweep(uint32_t sweepNum)
 {
	bool res = false; 				// stores the result
	MetaData metadata = {0}; // store the sweep metadata
	 
  // allocate memory for sweeps
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	int16_t * real = nrf_malloc(MAX_IMP_SIZE);
	int16_t * imag = nrf_malloc(MAX_IMP_SIZE);
		
	// get the sweep data from flash
	if (flashManager_getSweep(freq, real, imag, &metadata, sweepNum))
	{
		if (usbManager_sendSweep(freq, real, imag, &metadata))
		{
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep send from flash success");
			NRF_LOG_FLUSH();
#endif
			res = true;
		}
		else
		{
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep send fail");
			NRF_LOG_FLUSH();
#endif
		}
	}
	
	// free memory
	nrf_free(freq);
	nrf_free(real);
	nrf_free(imag);
	
	return res;
 }
 
// Executes a sweep and sends it over usb without saving
// Arguments: 
//	* sweep    : The sweep to execute
// Return value:
//  false if error sending sweep
//  true  if sweep sent successfully
 bool sensorFunctions_sweepAndSend(Sweep * sweep)
 {
	bool res = false; // stores the result 
	 
	// allocate memory for sweeps
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	int16_t * real = nrf_malloc(MAX_IMP_SIZE);
	int16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	 
#ifdef TESTING
	testFunctions_dummyData(freq, sizeof(uint32_t) * sweep->metadata.numPoints);
	testFunctions_dummyData(real, sizeof(uint16_t) * sweep->metadata.numPoints);
	res = testFunctions_dummyData(imag, sizeof(uint16_t) * sweep->metadata.numPoints);
#else
	res = AD5933_Sweep(sweep, freq, real, imag);
#endif
	
	// execute the sweep
	if (res)
	{
#ifdef DEBUG_FUNCTIONS
		NRF_LOG_INFO("FUNCTIONS: Sending Sweep");
#endif
		if (usbManager_sendSweep(freq, real, imag, &sweep->metadata))
		{
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep Send Success");
#endif
			res = true;
		}
		else
		{
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep Send Fail");
#endif
			res = false;
		}
#ifdef DEBUG_FUNCTIONS
		NRF_LOG_FLUSH();
#endif
	}
	
	// free the memory
	nrf_free(freq);
	nrf_free(real);
	nrf_free(imag);
	
	return res;
 }
 
// Executes a sweep and saves it to flash
// Arguments: 
//	* sweep    : The sweep to execute
//	* numSaved : The number of sweeps currently saved
 // 	usb			 : If true, sends confimration over usb that sweep was successful
// Return value:
//  false if error saving sweep
//  true  if sweep saved successfully
 bool sensorFunctions_saveSweep(Config * config, bool usb)
{
	// allocate memory for sweep data
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	int16_t * real = nrf_malloc(MAX_IMP_SIZE);
	int16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	
	bool res = false; // to save if success
	
	uint8_t buff[1] = {1}; // to save the result for usb

	if (AD5933_Sweep(&config->sweep, freq, real, imag))
	{
		if (flashManager_saveSweep(freq, real, imag, &config->sweep.metadata, config->num_sweeps + 1))
		{
			
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep %d saved", config->num_sweeps);
			NRF_LOG_FLUSH();
#endif
			res = true;
			buff[0] = 2;
		}
		else
		{
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep save fail");
			NRF_LOG_FLUSH();
#endif
		}
	}
	else
	{
#ifdef DEBUG_FUNCTIONS
		NRF_LOG_INFO("FUNCTIONS: Sweep save fail");
		NRF_LOG_FLUSH();
#endif
	}
	
	// send result over usb if usb is true
	if (usb) usbManager_writeBytes(buff, 1);
	
	// free the memory
	nrf_free(freq);
	nrf_free(real);
	nrf_free(imag);
	
	return res;
}

// Deletes all the saved sweeps in flash and reset the number of saved sweeps
// Arguments: 
//	numSaved: the number of sweeps saved to flash
//	numSaved: the number of sweeps that have been deleted
//	usb			: true to send a confirmation over usb
// Return value:
//  false if error deleting sweeps
//  true  if delete successful
bool sensorFunctions_deleteSweeps(Config * config, bool usb)
{
	bool stat = true;
	
#ifdef DEBUG_LOG
	NRF_LOG_INFO("Deleting all sweeps");
	NRF_LOG_FLUSH();
#endif
	
	// delete all the sweeps
	while (config->num_sweeps > 0 && stat)
	{
		stat = flashManager_deleteFile(config->num_sweeps);
		if (stat)
		{
			config->num_sweeps--;
			config->num_deleted++;
		}
	}
	
	if (stat && config->num_sweeps == 0)
	{
#ifdef DEBUG_LOG
		NRF_LOG_INFO("FUNCTIONS: All sweeps deleted");
		NRF_LOG_FLUSH();
#endif
	}
	else
	{
#ifdef DEBUG_LOG
		NRF_LOG_INFO("FUNCTIONS: Sweeps delete fail");
		NRF_LOG_FLUSH();
#endif
	}
	
	// update the config
	flashManager_updateConfig(config);
	
	// if connected to usb send confirmation of success or failure
	if (usb)
	{
		uint8_t buff[1];
		if (stat) 
		{
			buff[0] = 5;
		}
		else
		{
			buff[0] = 6;
		}
		usbManager_writeBytes(buff, 1);
	}
	
	// return stat
	return stat;
}

void sensorFunctions_set_default(Sweep * sweep)
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

// inits all the required managers and peripherals for the wireless sensor
// Return value:
//  false if an error occured
//  true  if all inits successfull
bool sensorFunctions_init(void)
{
  // init Log
#ifdef DEBUG_FUNCTIONS
  NRF_LOG_INFO("FUNCTIONS: Wireless Sensor Started");
  NRF_LOG_FLUSH();
#endif
	
	// init BLE, does not complete the BLE init process until FreeRTOS task initiates
	bleManager_init();

  // init twi
  if (!twiManager_init()) return false;

#ifndef SOFTDEVICE_PRESENT
  // init USB
  if (!usbManager_init()) return false;
	
	// init flashManager
	if (!flashManager_init()) return false;
#endif
	
	// init memory manager
	if (nrf_mem_init() != NRF_SUCCESS) return false;
	
	// init gpiote
	if (!gpioteManager_init()) return false;
	
	// init saadc
	// if (!adcManager_init()) return false;
	
#ifdef DEBUG_FUNCTIONS
	NRF_LOG_INFO("FUNCTIONS: Init Peripherals Success");
  NRF_LOG_FLUSH();
#endif

#ifndef TESTING
  // reset the AD5933
  if (AD5933_SetControl(NO_OPERATION, RANGE1, GAIN1, INTERN_CLOCK, 1))
	{
		gpioteManager_writePin(LED_2, 2);
		nrf_delay_ms(200);
		gpioteManager_writePin(LED_2, 2);
	}
	else
	{
#ifdef DEBUG_FUNCTIONS
		NRF_LOG_INFO("FUNCTIONS: AD5933 Init Fail");
		NRF_LOG_FLUSH();
#endif
	}
#endif
	
	// success
	return true;
}
