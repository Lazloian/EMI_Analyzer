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
// Return value:
//  false if error
//  true  if success
 bool sensorFunctions_sendConfig(Sweep * sweep, uint32_t * numSaved)
 {
	// check and update from the config file
	if (!flashManager_checkConfig(numSaved, sweep)) return false;
	 
	// send the number of saved sweeps
	if (!usbManager_writeBytes(numSaved, sizeof(numSaved))) return false;
	 
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
	uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
	uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
		
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
	uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
	uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	
	// execute the sweep
	if (AD5933_Sweep(sweep, freq, real, imag))
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
// Return value:
//  false if error saving sweep
//  true  if sweep saved successfully
 bool sensorFunctions_saveSweep(Sweep * sweep, uint32_t * numSaved, bool usb)
{
	// allocate memory for sweep data
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
	uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	
	bool res = false; // to save if success
	
	uint8_t buff[1] = {1}; // to save the result for usb

	if (AD5933_Sweep(sweep, freq, real, imag))
	{
		if (flashManager_saveSweep(freq, real, imag, &sweep->metadata, *numSaved + 1))
		{
			*numSaved += 1;
			flashManager_updateNumSweeps(numSaved);
#ifdef DEBUG_FUNCTIONS
			NRF_LOG_INFO("FUNCTIONS: Sweep %d saved", *numSaved);
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

void sensorFunctions_set_default(Sweep * sweep)
{
  // set the default sweep parameters
  sweep->start 							= 1000;
  sweep->delta 							= 100;
  sweep->steps 							= 20;//490;
  sweep->cycles 						= 20;//511;
  sweep->cyclesMultiplier 	= NO_MULT;//TIMES4;
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

  // init twi
  if (!twiManager_init()) return false;

  // init USB
  if (!usbManager_init()) return false;
	
	// init flashManager
	if (!flashManager_init()) return false;
	
	// init memory manager
	if (nrf_mem_init()) return false;
	
	// init rtc
	//if (!rtcManager_init()) return false;
	
	// init gpiote
	if (!gpioteManager_init()) return false;
	
#ifdef DEBUG_FUNCTIONS
	NRF_LOG_INFO("FUNCTIONS: Init Peripherals Success");
  NRF_LOG_FLUSH();
#endif

  // reset the AD5933
  if (AD5933_SetControl(NO_OPERATION, RANGE1, GAIN1, INTERN_CLOCK, 1))
	{
		// blink all the LEDs
		gpioteManager_writePin(RAK_LED_1, 1);
		gpioteManager_writePin(RAK_LED_2, 1);
		nrf_delay_ms(200);
		gpioteManager_writePin(RAK_LED_1, 0);
		gpioteManager_writePin(RAK_LED_2, 0);
	}
	else
	{
#ifdef DEBUG_FUNCTIONS
	NRF_LOG_INFO("FUNCTIONS: AD5933 Init Fail");
  NRF_LOG_FLUSH();
#endif
	}
	
	// success
	return true;
}
