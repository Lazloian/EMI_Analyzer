/*
 *  testFunctions.c
 *
 *  Functions to make testing easier
 *
 *  Author: Henry Silva
 *
 */
 
 #include "testFunctions.h"
 
// Saves a dummy sweep to flash
// Arguments: 
//	* sweep    : The sweep to execute
//	* numSaved : The number of sweeps currently saved
// Return value:
//  false if error saving sweep
//  true  if sweep saved successfully
 bool testFunctions_saveDummy (Sweep * sweep, uint32_t * numSaved, bool usb)
{
	// allocate memory for sweep data
	uint32_t * freq = nrf_malloc(MAX_FREQ_SIZE);
	uint16_t * real = nrf_malloc(MAX_IMP_SIZE);
	uint16_t * imag = nrf_malloc(MAX_IMP_SIZE);
	
	bool res = false; // to save if success
	
	uint8_t buff[1] = {1}; // to save the result for usb
	
	// fill with dummy data
	testFunctions_dummyData(freq, sizeof(uint32_t) * sweep->metadata.numPoints);
	testFunctions_dummyData(real, sizeof(uint16_t) * sweep->metadata.numPoints);
	testFunctions_dummyData(imag, sizeof(uint16_t) * sweep->metadata.numPoints);

	if (flashManager_saveSweep(freq, real, imag, &sweep->metadata, *numSaved + 1))
	{
		*numSaved += 1;
		flashManager_updateNumSweeps(numSaved);
#ifdef DEBUG_TEST
		NRF_LOG_INFO("TEST: Dummy Sweep %d saved", *numSaved);
		NRF_LOG_FLUSH();
#endif
		res = true;
		buff[0] = 2;
	}
	else
	{
#ifdef DEBUG_TEST
		NRF_LOG_INFO("TEST: Dummy Sweep save fail");
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
 
// Fills an array of size num_bytes with dummy data
// Arguments: 
//	* buff	  - Pointer to the buffer to write the dummy data
//	num_bytes - The number of bytes to fill in
// Return value:
//  true if success
//	false if error
 bool testFunctions_dummyData(void * buff, uint32_t num_bytes)
 {
	 uint8_t * p = (uint8_t *) buff; // converted pointer to the buffer
	 uint8_t data = 1; 							// dummy data
	 
	 for (uint32_t i = 0; i < num_bytes; i++)
	 {
		 if (data == 255) data = 1;
		 p[i] = data;
		 data++;
	 }
	 
	 return true;
 }
