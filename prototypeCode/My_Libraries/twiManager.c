/*
 *  twiManager.c
 *
 *  Functions to init and handle twi events.
 *
 *  Author: Henry Silva
 *
 */
 
#include "twiManager.h"
 
 // get the TWI instance
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

// Indicates if operation on TWI has ended
static volatile bool m_xfer_done = false;

// Indicates TWI error
static volatile bool twi_error = false;

// Writes bytes to a register of a device over twi
// Arguments: 
//  address  - The 8 bit address of the twi device
//	* buff	 - Pointer to the buffer of the bytes to send
//	numBytes - The number of bytes to write
// Return value:
//  false if I2C error
//  true if no error
bool twiManager_write(uint8_t address, uint8_t * buff, uint8_t numBytes)
{
	// stores error code
  ret_code_t err_code;
	
#ifdef DEBUG_TWI
	NRF_LOG_INFO("TWI: Writing %d bytes to device %x", numBytes, address);
	NRF_LOG_FLUSH();
#endif
	
	// send the data
  m_xfer_done = false;
  err_code = nrf_drv_twi_tx(&m_twi, address, buff, numBytes, false);
	
	// check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;

  // success
  return true;
}

// Reads bytes from a register of a device over twi
// Arguments: 
//  address  - The 8 bit address of the twi device
//	* buff	 - Pointer to the buffer to store the bytes read
//	numBytes - The number of bytes to read
// Return value:
//  false if I2C error
//  true if no error
bool twiManager_read(uint8_t address, uint8_t * buff, uint8_t numBytes)
{
	// stores error code
  ret_code_t err_code;
	
#ifdef DEBUG_TWI
	NRF_LOG_INFO("TWI: Reading %d bytes from device %x", numBytes, address);
	NRF_LOG_FLUSH();
#endif
	
	// read the bytes
  m_xfer_done = false;
  err_code = nrf_drv_twi_rx(&m_twi, address, buff, numBytes);

  // check for error
  APP_ERROR_CHECK(err_code);

  // wait for transfer to be done
  while (m_xfer_done == false);

  // check if fail
  if (twi_error || (err_code != NRF_SUCCESS)) return false;
	
	// success
	return true;
}


// TWI events handler.
void twiManager_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
  switch (p_event -> type)
  {
    case NRF_DRV_TWI_EVT_DONE:
      if (p_event -> xfer_desc.type == NRF_DRV_TWI_XFER_RX)
      {
#ifdef DEBUG_TWI
        NRF_LOG_INFO("TWI Read Success");
        NRF_LOG_FLUSH();
#endif
      }
      else if (p_event -> xfer_desc.type == NRF_DRV_TWI_XFER_TX)
      {
#ifdef DEBUG_TWI
        NRF_LOG_INFO("TWI Write Success");
        NRF_LOG_FLUSH();
#endif
      }

      // set transfer done to true
      m_xfer_done = true;
      // set error to false
      twi_error = false;
      break;

    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
#ifdef DEBUG_TWI
      NRF_LOG_INFO("TWI Address Not Found");
      NRF_LOG_FLUSH();
#endif

      // set transfer done to true
      m_xfer_done = true;
      // set twi error to true
      twi_error = true;
      break;

    case NRF_DRV_TWI_EVT_DATA_NACK:
#ifdef DEBUG_TWI
      NRF_LOG_INFO("TWI Transfer Failed");
      NRF_LOG_FLUSH();
#endif

      // set transfer done to true
      m_xfer_done = true;
      // set twi error to true
      twi_error = true;
      break;

    default:
      break;
  }
}


// TWI (I2C) initialization.
void twiManager_init (void)
{
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_lm75b_config = {
    .scl                = ARDUINO_SCL_PIN,
    .sda                = ARDUINO_SDA_PIN,
    .frequency          = NRF_DRV_TWI_FREQ_100K,
    .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
    .clear_bus_init     = false
  };
  // this inits twi and sets the handler function
  err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twiManager_handler, NULL);
  APP_ERROR_CHECK(err_code);

  nrf_drv_twi_enable(&m_twi);
}
