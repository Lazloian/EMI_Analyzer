/*
 *  flashManager.c
 *
 *  Functions to interact with FDS to save data to flash, namely saving sweep data.
 *
 *  Author: Henry Silva
 *
 */
 
 #include "flashManager.h"
 
// --- My Variables ---
static uint8_t processing = 0; // Holds the number of FDS commands that are in the queue

// --- FDS Variables ---

/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
  "FDS_EVT_INIT",
  "FDS_EVT_WRITE",
  "FDS_EVT_UPDATE",
  "FDS_EVT_DEL_RECORD",
  "FDS_EVT_DEL_FILE",
  "FDS_EVT_GC",
};

// Flag to check fds initialization.
static bool volatile m_fds_initialized;

// --- User Functions ---

// Returns if the current queued FDS command is done or not
// Return value:
//  complete
bool flashManager_checkComplete(void)
{
	if (processing) 
	{
#ifdef DEBUG_FLASH
		NRF_LOG_INFO("FLASH: Processing is %d", processing);
		NRF_LOG_FLUSH();
#endif
		return false;
	}
	return true;
}

// Deletes a sweep from flash
// Arguments: 
//  sweep_num: the number of the sweep to delete
// Return value:
//  false if sweep delete fail
//  true  if sweep delete success
bool flashManager_deleteSweep(uint32_t sweep_num)
{
  // delete the whole file
  return flashManager_deleteFile(sweep_num);
}
// Gets a sweep from flash and stores the data to given arrays
// Arguments: 
//	* freq: pointer to the frequency data array
//	* real: pointer to the real impedance data array
//	* imag: pointer to the imaginary impedance data array
// Return value:
//  false if error getting sweep to flash
//  true  if sweep get success
bool flashManager_getSweep(uint32_t * freq, int16_t * real, int16_t * imag, MetaData * metadata, uint32_t sweep_num)
{
#ifdef DEBUG_FLASH
	NRF_LOG_INFO("FLASH: Starting get sweep");
	NRF_LOG_FLUSH();
#endif

  // create a new record desc
  fds_record_desc_t record_desc;
	
	// find the sweep metadata
  if (!flashManager_findRecord(&record_desc, sweep_num, SWEEP_METADATA)) return false;

  // copy the metadata
  if (!flashManager_readRecord(&record_desc, metadata, sizeof(MetaData))) return false;

	// find the sweep frequency data
  if (!flashManager_findRecord(&record_desc, sweep_num, SWEEP_FREQ)) return false;

  // copy the frequency data
  if (!flashManager_readRecord(&record_desc, freq, sizeof(uint32_t) * metadata->numPoints)) return false;

	// find the sweep real impedance data
  if (!flashManager_findRecord(&record_desc, sweep_num, SWEEP_REAL)) return false;

  // copy the frequency data
  if (!flashManager_readRecord(&record_desc, real, sizeof(int16_t) * metadata->numPoints)) return false;

	// find the sweep real impedance data
  if (!flashManager_findRecord(&record_desc, sweep_num, SWEEP_IMAG)) return false;

  // copy the frequency data
  if (!flashManager_readRecord(&record_desc, imag, sizeof(int16_t) * metadata->numPoints)) return false;
	
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Sweep get success");
	NRF_LOG_FLUSH();
#endif
	
	// success
	return true;
}

// Saves sweep data to flash in a new file
// Arguments: 
//	* freq: pointer to the frequency data array
//	* real: pointer to the real impedance data array
//	* imag: pointer to the imaginary impedance data array
// Return value:
//  false if error saving sweep to flash
//  true  if sweep saved successfully
bool flashManager_saveSweep(uint32_t * freq, int16_t * real, int16_t * imag, MetaData * metadata, uint32_t sweep_num)
{
#ifdef DEBUG_FLASH
	NRF_LOG_INFO("FLASH: Starting save sweep %d", sweep_num);
	NRF_LOG_FLUSH();
#endif

  fds_record_desc_t record_desc;
	
	// check if the max number of files has been reached (file num happens to the sweep num)
	if (sweep_num > FM_MAX_FILES)
	{
#ifdef DEBUG_FLASH
		NRF_LOG_INFO("FLASH: Max Number of Sweeps Reached. Not Saving", sweep_num);
		NRF_LOG_FLUSH();
#endif
		return false;
	}

  // save frequency data
  if (!flashManager_createRecord(&record_desc, sweep_num, SWEEP_FREQ, freq, metadata->numPoints * sizeof(uint32_t))) return false;
	
  // save real data
  if (!flashManager_createRecord(&record_desc, sweep_num, SWEEP_REAL, real, metadata->numPoints * sizeof(int16_t))) return false;

  // save imaginary data
  if (!flashManager_createRecord(&record_desc, sweep_num, SWEEP_IMAG, imag, metadata->numPoints * sizeof(int16_t))) return false;

  // save metadata
  if (!flashManager_createRecord(&record_desc, sweep_num, SWEEP_METADATA, metadata, sizeof(MetaData))) return false;
	
#ifdef DEBUG_FLASH
	NRF_LOG_INFO("FLASH: Sweep save success");
	NRF_LOG_FLUSH();
#endif
	
	// success
	return true;
}

// updates the config record with a given config
// Arguments: 
//  * config: pointer to the config struct to save
// Return value:
//  false if error updating config file
//  true  if config file updated successfully
bool flashManager_updateConfig(Config * config)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Updating the config file");
	NRF_LOG_INFO("FLASH: Saved sweeps: %d", config->num_sweeps);
  NRF_LOG_INFO("FLASH: Deleted sweeps: %d", config->num_deleted);
  NRF_LOG_INFO("FLASH: Sent sweeps: %d", config->num_sent);
	NRF_LOG_INFO("FLASH: Device ID: %c%c%c", config->device_id[0], config->device_id[1], config->device_id[2]);
  NRF_LOG_FLUSH();
#endif

  // create a record desc to save the record
  fds_record_desc_t record_desc;

  // find the number of saved sweeps record
  if (!flashManager_findRecord(&record_desc, CONFIG_ID, CONFIG_RECORD)) return false;
  
  // update the record
  if (!flashManager_updateRecord(&record_desc, CONFIG_ID, CONFIG_RECORD, config, sizeof(Config))) return false;

	// update success
	return true;
}

// checks for config files and loads the number of saved sweep and the saved sweep parameters from flash
// if no config file is found, a new config file is created with the config at the pointer
// Arguments: 
//  * config: pointer to a config struct to load the data into
// Return value:
//  false if error loading config file
//  true  if config file loaded successfully
bool flashManager_checkConfig(Config * config)
{
  // record desc to store records
  fds_record_desc_t record_desc;

  // try to find the config record
  if (flashManager_findRecord(&record_desc, CONFIG_ID, CONFIG_RECORD))
  {
    // config record found, get its contents
    if (!flashManager_readRecord(&record_desc, config, sizeof(Config))) return false;
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Config loaded successfully:");
    NRF_LOG_INFO("FLASH: Saved sweeps: %d", config->num_sweeps);
    NRF_LOG_INFO("FLASH: Deleted sweeps: %d", config->num_deleted);
    NRF_LOG_INFO("FLASH: Sent sweeps: %d", config->num_sent);
    NRF_LOG_FLUSH();
#endif
  }
  else
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Config record not found, creating one");
    NRF_LOG_FLUSH();
#endif
    // num sweeps record not found, create one
    if (!flashManager_createRecord(&record_desc, CONFIG_ID, CONFIG_RECORD, config, sizeof(Config))) return false;
  }
	
  // success
	return true;
}

// Initializes FDS
// Return value:
//  false if error with starting FDS
//  true  if FDS started successfully
bool flashManager_init(void)
{
	ret_code_t ret;	 // NRF status
	fds_stat_t stat; // FDS status
	
	// Register first to receive an event when initialization is complete.
  (void) fds_register(fds_evt_handler);

  // init fds and check for error
	processing++;
  ret = fds_init();
  APP_ERROR_CHECK(ret);
  // Wait for fds to initialize.
  //wait_for_fds_ready();
	
	/*
	// check FDS status
	ret = fds_stat(&stat);
  APP_ERROR_CHECK(ret);
	*/
	
	return true;
}

// --- FDS Helper Functions ---

// Creates a record with given parameters
// Arguments:
//  * record_desc: Pointer to the record desc
//  file_id:    The file ID to store the record
//  record_key: The record key
//  * p_data:   Pointer to the data to store
//  num_bytes:  The number of bytes of the data
// Returns:
//  true if record write success
//  false if record write fail
static bool flashManager_createRecord(fds_record_desc_t * record_desc, uint32_t file_id, uint32_t record_key, void const * p_data, uint32_t num_bytes)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Writing %d bytes to record %d in file %d", num_bytes, record_key, file_id);
  NRF_LOG_FLUSH();
#endif

	ret_code_t ret;	 // NRF status

  // create a new record
  fds_record_t record;

  record.file_id           = file_id;
  record.key               = record_key;
  record.data.p_data       = p_data;
  record.data.length_words = (num_bytes + 3) / 4; // account for remainder
  
  // queue write record
	processing++;
  ret = fds_record_write(record_desc, &record);
  
  // check if flash full
  if ((ret != NRF_SUCCESS) && (ret == FDS_ERR_NO_SPACE_IN_FLASH))
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Flash full, write record fail", record_key, file_id);
    NRF_LOG_FLUSH();
#endif
		processing--;
    return false;
  } // if fail, but not flash full then it is a bigger problem
  else
  {
    APP_ERROR_CHECK(ret);
  }

#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Write record success", record_key, file_id);
  NRF_LOG_FLUSH();
#endif
  return true;
}
// Updates a record at record_desc with new information. Must find the record with flashManager_findRecord
// Arguments:
//  * record_desc: Pointer to the record desc to update
//  file_id:       The file ID to store the record
//  record_key:    The record key
//  * p_data:      Pointer to the data to store
//  num_bytes:     The number of bytes of the data
// Returns:
//  true if record write success
//  false if record write fail
static bool flashManager_updateRecord(fds_record_desc_t * record_desc, uint32_t file_id, uint32_t record_key, void const * p_data, uint32_t num_bytes)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Updating record %d at file %d", record_key, file_id);
  NRF_LOG_FLUSH();
#endif

	ret_code_t ret;	 // NRF status

  // create a new record
  fds_record_t record;

  record.file_id           = file_id;
  record.key               = record_key;
  record.data.p_data       = p_data;
  record.data.length_words = (num_bytes + 3) / 4; // account for remainder
  
  // queue the record update
	processing++;
  ret = fds_record_update(record_desc, &record);
  
  // check if flash full
  if ((ret != NRF_SUCCESS) && (ret == FDS_ERR_NO_SPACE_IN_FLASH))
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Flash full, write record fail", record_key, file_id);
    NRF_LOG_FLUSH();
#endif
		processing--;
    return false;
  } // if fail, but not flash full then it is a bigger problem
  else
  {
    APP_ERROR_CHECK(ret);
  }

#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Write record success", record_key, file_id);
  NRF_LOG_FLUSH();
#endif
  return true;
}

// Opens a record at record_desc and copies its contents to the buffer
// Arguments:
//  * record_desc: Pointer to store the found record desc
//  * buff:          Buffer to store the flash data
//  num_bytes:     Number of bytes to read from flash
// Returns:
//  true if flash read success
//  false if flash read fail
static bool flashManager_readRecord(fds_record_desc_t * record_desc, void * buff, uint32_t num_bytes)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Reading from record");
  NRF_LOG_FLUSH();
#endif
  // create a new flash_record struct
  fds_flash_record_t flash_record;

  // ret code to store result
  ret_code_t ret;
  
  // open the record
  ret = fds_record_open(record_desc, &flash_record);
  // check if fail
  if (ret != NRF_SUCCESS) 
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Error opening file: %s", fds_err_str(ret));
    NRF_LOG_FLUSH();
#endif
    return false;
  }

  // copy the data from the record to the buffer
  memcpy(buff, flash_record.p_data, num_bytes);

  // close the file
  if (fds_record_close(record_desc) != NRF_SUCCESS) return false;

#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Read success");
  NRF_LOG_FLUSH();
#endif

  // success
  return true;
}

// Finds a record given file ID and record key
// Arguments:
//  * record_desc: Pointer to store the found record desc
//  file_id:       The file ID to find the record in
//  record_key:    The record key to find
// Returns:
//  true if record found
//  false if record not found
static bool flashManager_findRecord(fds_record_desc_t * record_desc, uint32_t file_id, uint32_t record_key)
{
  // create a new find token and zero it
  fds_find_token_t ftok;
  memset(&ftok, 0x00, sizeof(fds_find_token_t));

#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Looking for record %d in file %d", record_key, file_id);
  NRF_LOG_FLUSH();
#endif
  // find the record, if fail return false
  if (fds_record_find(file_id, record_key, record_desc, &ftok) == NRF_SUCCESS)
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Record found");
    NRF_LOG_FLUSH();
#endif
    return true;
  }
  else
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Record not found");
    NRF_LOG_FLUSH();
#endif
    return false;
  }
}

// Deletes a record given a record description
// Arguments:
//  * record_desc: Pointer to the record description
// Returns:
//  true if record delete success
//  false if record delete fail
static bool flashManager_deleteRecord(fds_record_desc_t * record_desc)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Deleting record");
  NRF_LOG_FLUSH();
#endif
	
	// queue record delete
	processing++;
  if (fds_record_delete(record_desc) != NRF_SUCCESS)
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Record delete fail");
    NRF_LOG_FLUSH();
#endif
		processing--;
    return false;
  }
	
  // success
  return true;
}

// Deletes a file given a file ID
// Arguments:
//  file_id: file id to delete
// Returns:
//  true if file delete success
//  false if file delete fail
bool flashManager_deleteFile(uint32_t file_id)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Deleting file %d", file_id);
  NRF_LOG_FLUSH();
#endif
	
	// queue file delete
	processing++;
  if (fds_file_delete(file_id) != NRF_SUCCESS)
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: File delete fail");
    NRF_LOG_FLUSH();
#endif
		processing--;
    return false;
  }
	
  // success
  return true;
}

// Runs garbage collection
// Returns:
//  true if collection success
//  false if collection fail
bool flashManager_collectGarbage(void)
{
#ifdef DEBUG_FLASH
  NRF_LOG_INFO("FLASH: Running garbage collection");
  NRF_LOG_FLUSH();
#endif
	processing++;
  if (fds_gc() != NRF_SUCCESS)
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Garbage collection fail");
    NRF_LOG_FLUSH();
#endif
		processing--;
    return false;
  }
  // success
  return true;
}

// --- FDS Functions ---

const char *fds_err_str(ret_code_t ret)
{
  /* Array to map FDS return values to strings. */
  static char const * err_str[] =
  {
    "FDS_ERR_OPERATION_TIMEOUT",
    "FDS_ERR_NOT_INITIALIZED",
    "FDS_ERR_UNALIGNED_ADDR",
    "FDS_ERR_INVALID_ARG",
    "FDS_ERR_NULL_ARG",
    "FDS_ERR_NO_OPEN_RECORDS",
    "FDS_ERR_NO_SPACE_IN_FLASH",
    "FDS_ERR_NO_SPACE_IN_QUEUES",
    "FDS_ERR_RECORD_TOO_LARGE",
    "FDS_ERR_NOT_FOUND",
    "FDS_ERR_NO_PAGES",
    "FDS_ERR_USER_LIMIT_REACHED",
    "FDS_ERR_CRC_CHECK_FAILED",
    "FDS_ERR_BUSY",
    "FDS_ERR_INTERNAL",
  };

  return err_str[ret - NRF_ERROR_FDS_ERR_BASE];
}

// FDS events handler
static void fds_evt_handler(fds_evt_t const * p_evt)
{
  if (p_evt->result == NRF_SUCCESS)
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Event: %s received (NRF_SUCCESS)",
        fds_evt_str[p_evt->id]);
#endif
  }
  else
  {
#ifdef DEBUG_FLASH
    NRF_LOG_INFO("FLASH: Event: %s received (%s)",
        fds_evt_str[p_evt->id],
        fds_err_str(p_evt->result));
#endif
  }

  switch (p_evt->id)
  {
    case FDS_EVT_INIT:
      if (p_evt->result == NRF_SUCCESS)
      {
        processing--;//m_fds_initialized = true;
      }
      break;

    case FDS_EVT_WRITE:
      {
        if (p_evt->result == NRF_SUCCESS)
        {
#ifdef DEBUG_FLASH
          NRF_LOG_INFO("FLASH: Record ID:\t0x%04x",  p_evt->write.record_id);
          NRF_LOG_INFO("FLASH: File ID:\t0x%04x",    p_evt->write.file_id);
          NRF_LOG_INFO("FLASH: Record key:\t0x%04x", p_evt->write.record_key);
#endif
        }
				processing--;
      } break;
			
		case FDS_EVT_UPDATE:
		{
			processing--;
		} break;
		
		case FDS_EVT_DEL_FILE:
		{
			processing--;
		} break;
		
		case FDS_EVT_GC:
		{
			processing--;
			break;
		}

    case FDS_EVT_DEL_RECORD:
      {
        if (p_evt->result == NRF_SUCCESS)
        {
#ifdef DEBUG_FLASH
          NRF_LOG_INFO("FLASH: Record ID:\t0x%04x",  p_evt->del.record_id);
          NRF_LOG_INFO("FLASH: File ID:\t0x%04x",    p_evt->del.file_id);
          NRF_LOG_INFO("FLASH: Record key:\t0x%04x", p_evt->del.record_key);
#endif
        }
				processing--;
      } break;

    default:
      break;
  }
#ifdef DEBUG_FLASH
	NRF_LOG_INFO("FLASH: Processing %d", processing);
	NRF_LOG_FLUSH();
#endif
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
  while (!m_fds_initialized)
  {
    __WFE();
  }
}
