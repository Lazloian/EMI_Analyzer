/*
 *  AD5933.c
 *
 *  Functions to control the AD5933. All conversions are done within the functions.
 *
 *  Author: Henry Silva
 *
 */

#include "AD5933.h"

// sweeps given sweep parameters, outputs the data over USB
// Arguments: 
//	* sweep: pointer to the sweep struct
// Return value:
//  false if error with starting sweep
//  true  if sweep started successfully
bool AD5933_Sweep(Sweep * sweep)
{
  // set the range, gain, clock source, and reset the AD5933
  // Although reseting the AD5933 puts it in standby mode (according to the datasheet), 
  // sending a reset command along with a no operation command will put the AD5933
  // in no operation mode upon reset instead of standby mode
  if (!AD5933_SetControl(STANDBY, sweep->range, sweep->gain, sweep->clockSource, 1)) return false;

  // set the start frequency
  if (!AD5933_SetStart(sweep->start, sweep->clockFrequency)) return false;

  // set the delta frequency
  if (!AD5933_SetDelta(sweep->delta, sweep->clockFrequency)) return false;

  // set the number of steps
  if (!AD5933_SetSteps(sweep->steps)) return false;

  // set the number of cycles
  if (!AD5933_SetCycles(sweep->cycles, sweep->cyclesMultiplier)) return false;

  // initialize sweep with start frequency (AD5933 should already be in standby mode from the reset earlier)
  if (!AD5933_SetControl(INIT_START_FREQ, sweep->range, sweep->gain, sweep->clockSource, 0)) return false;

  // delay 100 ms, this should be more than enough settling time
  nrf_delay_ms(100);

  // start the frequency sweep
  if (!AD5933_SetControl(START_SWEEP, sweep->range, sweep->gain, sweep->clockSource, 0)) return false;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sweep start success");
  NRF_LOG_FLUSH();
#endif

  // reset current sweep values
  sweep->currentStep = 0;
  sweep->currentFrequency = sweep->start;

  // start getting the data from the sweep
  bool i2c_stats = true; // track twi success
  uint8_t AD5933_status; // stores the AD5933 status
	
	// check AD5933 status before sweep
	i2c_stats = AD5933_ReadStatus(&AD5933_status);

  // read impedance data until sweep is complete or twi fail
  while (((AD5933_status & STATUS_DONE) != STATUS_DONE) && i2c_stats)
  {
    // wait till measurement is done
    do{
      i2c_stats = AD5933_ReadStatus(&AD5933_status);
      nrf_delay_ms(10);
    } while (((AD5933_status & STATUS_DATA) != STATUS_DATA) && i2c_stats);

    // read the impedance data
    i2c_stats = AD5933_ReadData(sweep->currentData);

    // send the data over USB in this order: frequency, real, imaginary
    uint8_t buff[8];

    // cut up currentFrequency into bytes
    uint8_t * sel = (uint8_t*) &sweep->currentFrequency;
    buff[0] = sel[0];
    buff[1] = sel[1];
    buff[2] = sel[2];
    buff[3] = sel[3];

    // copy the real and imaginary impedance values
    buff[4] = sweep->currentData[0];
    buff[5] = sweep->currentData[1];
    buff[6] = sweep->currentData[2];
    buff[7] = sweep->currentData[3];

    // send all the data over usb
    app_usbd_cdc_acm_write(&m_app_cdc_acm, buff, 8);

    // increment the sweep
    i2c_stats = AD5933_SetControl(INCREMENT_FREQ, sweep->range, sweep->gain, sweep->clockSource, 0);
    // update sweep status
    sweep->currentStep += 1;
    sweep->currentFrequency += sweep->delta;

    // check if the sweep is complete
    i2c_stats = AD5933_ReadStatus(&AD5933_status);
  }

  // sweep is done, put the AD5933 in power down mode
  i2c_stats = AD5933_SetControl(POWER_DOWN, sweep->range, sweep->gain, sweep->clockSource, 0);
  // reset sweep counters
  sweep->currentStep = 0;
  sweep->currentFrequency = sweep->start;

  // sweep success
  return true;
}

// sets the start frequency of the frequency sweep
// Arguments: 
//	start - The start frequency in Hz (1kHz - 100kHz)
// Return value:
//  false if I2C error or start frequency out of range
//  true if no error
bool AD5933_SetStart(uint32_t start, uint32_t clkFreq)
{
  // check if start is outside output frequency range
  if (start < 1000 || start > 100000) return false;

  // create buffer to store frequency bytes
  uint8_t buff[3];

  // calculate start frequency code
  start = ((float) start / (clkFreq / CLK_DIV)) * pow(2, 27);

  // cut code into 3 uint8_ts (most significant 4 bits not needed)
  buff[0] = (start >> 16) & 0x0F;
  buff[1] = (start >> 8) & 0xFF;
  buff[2] = start & 0xFF;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sending 0x%x%x%x to register 0x%x", buff[0], buff[1], buff[2], START_FREQ_REG);
  NRF_LOG_FLUSH();
#endif

  // write the buffer to the start frequency register
  if (!AD5933_WriteBytes(buff, 3, START_FREQ_REG)) return false;

  return true;
}

// sets the delta (increment) frequency of the frequency sweep
// Arguments: 
//	delta - The delta frequency in Hz (0Hz - 100kHz)
// Return value:
//  false if I2C error or delta frequency out of range
//  true if no error
bool AD5933_SetDelta(uint32_t delta, uint32_t clkFreq)
{
  // check if delta is less than 100kHz
  if (delta > 100000) return false;

  // create buffer to store frequency bytes
  uint8_t buff[3];

  // calculate delta frequency code
  delta = ((float) delta / (clkFreq / CLK_DIV)) * pow(2, 27);

  // cut code into 3 uint8_ts (most significant 4 bits not needed)
  buff[0] = (delta >> 16) & 0x0F;
  buff[1] = (delta >> 8) & 0xFF;
  buff[2] = delta & 0xFF;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sending 0x%x%x%x to register 0x%x", buff[0], buff[1], buff[2], DELTA_FREQ_REG);
  NRF_LOG_FLUSH();
#endif

  // write the buffer to the delta frequency register
  if (!AD5933_WriteBytes(buff, 3, DELTA_FREQ_REG)) return false;

  return true;
}

// sets the number of steps (increments) of the frequency sweep
// Arguments: 
//	steps - the number of steps (0 - 511)
// Return value:
//  false if I2C error or steps out of range
//  true if no error
bool AD5933_SetSteps(uint16_t steps)
{
  // check if number of steps is within range
  if (steps > 511) return false;

  // create buffer to store data
  uint8_t buff[2];

  // split steps into the buffer
  buff[0] = (steps >> 8) & 0x01;
  buff[1] = steps & 0xFF;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sending 0x%x%x to register 0x%x", buff[0], buff[1], NUM_STEPS_REG);
  NRF_LOG_FLUSH();
#endif

  // write the buffer to the number of steps register
  if (!AD5933_WriteBytes(buff, 2, NUM_STEPS_REG)) return false;

  return true;
}

// sets the number of settling times cycles between measurements
// Arguments: 
//	cycles     - the number of time cycles (1 - 511)
//	multiplier - multiplier for the number of time cycles (NO_MULT, TIMES2, TIMES4)
// Return value:
//  false if I2C error or cycles out of range
//  true if no error
bool AD5933_SetCycles(uint16_t cycles, uint8_t multiplier)
{
  // check if the number of cycles is less than 511
  if (cycles > 511) return false;

  // create buffer to store data
  uint8_t buff[2];

  // assign bytes to buffer
  buff[0] = ((cycles >> 8) & 0x01) | multiplier;
  buff[1] = cycles & 0xFF;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sending 0x%x%x to register 0x%x", buff[0], buff[1], NUM_CYCLES_REG);
  NRF_LOG_FLUSH();
#endif

  // write the buffer to the time cycles register
  if (!AD5933_WriteBytes(buff, 2, NUM_CYCLES_REG)) return false;

  // success
  return true;
}

// sets the control registers of the AD5933
// Arguments: 
//  command - the control function of the AD5933
//	range   - output excitation voltage range
//	gain		- PGA gain of the response signal
//	clock		- selects the internal or external clock
//	reset   - reset command (1 for reset, 0 for no reset)
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_SetControl(uint8_t command, uint8_t range, uint8_t gain, uint8_t clock, uint8_t reset)
{
  // create buffer to store commands
  uint8_t buff[2];

  // combine the commands
  buff[0] = (command << 4) | ((range << 2) | gain);
  buff[1] = (reset << 4) | (clock << 3);

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Sending 0x%x to register 0x%x and 0x%x to register 0x%x", buff[0], CONTROL1_REG, buff[1], CONTROL2_REG);
  NRF_LOG_FLUSH();
#endif

  // write to each register one at a time (the datasheet specifies that a block write command cannot be used)
  // check for error
  if (!AD5933_Write(buff[0], CONTROL1_REG)) return false;
  return AD5933_Write(buff[1], CONTROL2_REG);
}

// reads the status of the AD5933 and puts the data into buff
// Arguments: 
//  buff - Pointer to the buffer that stores the data
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_ReadStatus(uint8_t * buff)
{
  // saves result of byte read
  bool status;

  // read the status byte
  status = AD5933_ReadBytes(buff, 1, STATUS_REG);

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Read 0x%x from register 0x%x", buff[0], STATUS_REG);
  NRF_LOG_FLUSH();
#endif

  // return status
  return status;
}

// reads the temperature register of the AD5933 and converts it to celcius. Must first send a Measure Temperature command via AD5933_SetControl
// Arguments: 
//  * temp - the pointer to the integer that will store the temperature in Celcius
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_ReadTemp(int * temp)
{
  // create buffer to store temperature data
  uint8_t buff[2];

  // read the temperature register
  AD5933_ReadBytes(buff, 2, TEMP_REG);

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Read 0x%x%x from register 0x%x", buff[0], buff[1], TEMP_REG);
  NRF_LOG_FLUSH();
#endif

  // check if temp is negative
  if ((buff[0] & 0x20))
  {
    *temp = -1;
  }
  else
  {
    // put the data into a 16 bit unsigned int
    uint16_t num = buff[0];
    num = (num << 8) | buff[1];

    // calculate
    *temp = num / 32;
  }
  return true;
}

// Reads the data located in the real and imaginary data registers
// Arguments: 
//  * data - buffer to store 16 bit data values (imaginary and real)
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_ReadData(uint8_t * data)
{
  // read the data from the data register
  if (!AD5933_ReadBytes(data, 2, REAL_REG)) return false;
  if (!AD5933_ReadBytes(&data[2], 2, IMAG_REG)) return false;

  // success
  return true;
}

// TWI helper functions

// Reads numbytes starting at register reg
// Arguments: 
//  buff     - pointer to the array to store the read data
//  numbytes - the number of bytes to read
//  reg      - The register to read from
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_ReadBytes(uint8_t * buff, uint8_t numbytes, uint8_t reg)
{
  // set the pointer and check for failure
  if (!AD5933_SetPointer(reg)) return false;

  // block read numbytes from the pointer
  if (!AD5933_BlockRead(buff, numbytes)) return false;

  // success
  return true;
}

// Writes numbytes starting at register reg
// Arguments: 
//  buff     - pointer to the array to the data to write
//  numbytes - the number of bytes to write
//  reg      - The register to write to
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_WriteBytes(uint8_t * buff, uint8_t numbytes, uint8_t reg)
{
  // set the pointer and check for failure
  if (!AD5933_SetPointer(reg)) return false;

  // block 2 bytes from the temperature register
  if (!AD5933_BlockWrite(buff, numbytes)) return false;

  // success
  return true;
}

// Sets the internal pointer of the AD5933. Usually do this before block read or write
// Arguments: 
//  reg - The register to point to
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_SetPointer(uint8_t reg)
{
  // stores error code
  ret_code_t err_code;

  // set data buffer
  uint8_t buff[2] = {SET_POINTER, reg};

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Setting Pointer to %x", reg);
  NRF_LOG_FLUSH();
#endif

  // send the data
  m_xfer_done = false;
  err_code = nrf_drv_twi_tx(&m_twi, AD5933_ADDR, buff, sizeof(buff), false);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;
}

// Arguments:
//  reg  - The register to write to
//  data - The uint8_t to write
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_Write(uint8_t data, uint8_t reg)
{
  // stores error code
  ret_code_t err_code;

  // set data buffer
  uint8_t buff[2] = {reg, data};

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Writing 0x%x to register 0x%x", data, reg);
  NRF_LOG_FLUSH();
#endif

  // send the data
  m_xfer_done = false;
  err_code = nrf_drv_twi_tx(&m_twi, AD5933_ADDR, buff, sizeof(buff), false);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;
}

// Write numbytes (max 30) to the location of the internal pointer (set the pointer with AD5933_SetPointer)
// Arguments:
//  buff     - Pointer to uint8_t array to write from
//  numbytes - Number of bytes to write
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_BlockWrite(uint8_t * buff, uint8_t numbytes)
{
  // stores error code
  ret_code_t err_code;

  // check if numbytes greater than 32
  if (numbytes > 30)
  {
    return false;
  }

  // create data buffer. 
  uint8_t data[32];

  // set block write and number of bytes
  data[0] = BLOCK_WRITE;
  data[1] = numbytes;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Block Writing %d bytes to pointer", numbytes);
  NRF_LOG_FLUSH();
#endif

  // set the rest of the data
  for (int i = 0; i < numbytes; i++)
  {
    data[i + 2] = buff[i];
  }

  // send the data
  m_xfer_done = false;
  err_code = nrf_drv_twi_tx(&m_twi, AD5933_ADDR, data, numbytes + 2, false);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;	
}

// Read one byte from the location of the internal pointer (set the pointer with AD6933_setPointer)
// Arguments:
//  buff     - Pointer to the uint8_t to input the read byte
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_ReadByte(uint8_t * buff)
{
  // stores error code
  ret_code_t err_code;

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Reading a byte from pointer");
  NRF_LOG_FLUSH();
#endif

  // read byte from AD5933
  m_xfer_done = false;
  err_code = nrf_drv_twi_rx(&m_twi, AD5933_ADDR, buff, 1);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;
}

// Read numbytes from the location of the internal pointer (set the pointer with AD6933_setPointer)
// Arguments:
//  buff     - Pointer to the uint8_t array to input the read data
//  numbytes - The number of uint8_ts to read
// Return value:
//  false if I2C error
//  true if no error
bool AD5933_BlockRead(uint8_t * buff, uint8_t numbytes)
{
  // stores error code
  ret_code_t err_code;

  // set data buffer
  uint8_t data[2] = {BLOCK_READ, numbytes};

#ifdef DEBUG_LOG
  NRF_LOG_INFO("Block reading %d bytes from pointer", numbytes);
  NRF_LOG_FLUSH();
#endif

  // send the data to initiate block read
  m_xfer_done = false;
  err_code = nrf_drv_twi_tx(&m_twi, AD5933_ADDR, data, sizeof(data), false);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // now read numbytes from the AD5933
  m_xfer_done = false;
  err_code = nrf_drv_twi_rx(&m_twi, AD5933_ADDR, buff, numbytes);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;
}
