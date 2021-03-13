/*
 *  flashManager.h
 *  
 *  Header file for flashManager.c
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_FLASHMANAGER_H_
#define INC_FLASHMANAGER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "app_error.h"
#include "app_timer.h"
#include "fds.h"

#include "AD5933.h"

#ifdef DEBUG_FLASH
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

// defines
#define CONFIG_ID         0x0000
#define CONFIG_NUM_SWEEPS 0x0001
#define CONFIG_SWEEP      0x0002
#define SWEEP_FREQ				0x0001
#define SWEEP_REAL				0x0002
#define SWEEP_IMAG				0x0003
#define SWEEP_METADATA		0x0004
#define MAX_FREQ_SIZE     2048
#define MAX_IMP_SIZE      1024

// User Functions
bool flashManager_init(void);
bool flashManager_checkConfig(uint32_t * num_sweeps, Sweep * sweep);
bool flashManager_saveSweep(uint32_t * freq, uint16_t * real, uint16_t * imag, MetaData * metadata, uint32_t sweep_num);
bool flashManager_getSweep(uint32_t * freq, uint16_t * real, uint16_t * imag, MetaData * metadata, uint32_t sweep_num);
bool flashManager_updateSavedSweep(Sweep * sweep);
bool flashManager_updateNumSweeps(uint32_t * num_sweeps);
bool flashManager_deleteSweep(uint32_t sweep_num);
bool flashManager_deleteAllSweeps(uint32_t num_sweeps);

// FDS helper functions
static bool flashManager_createRecord(fds_record_desc_t * record_desc, uint32_t file_id, uint32_t record_key, void const * p_data, uint32_t num_bytes);
static bool flashManager_findRecord(fds_record_desc_t * record_desc, uint32_t file_id, uint32_t record_key);
static bool flashManager_updateRecord(fds_record_desc_t* record_desc, uint32_t file_id, uint32_t record_key, void const * p_data, uint32_t num_bytes);
static bool flashManager_readRecord(fds_record_desc_t * record_desc, void * buff, uint32_t num_bytes);
static bool flashManager_deleteRecord(fds_record_desc_t * record_desc);
bool flashManager_deleteFile(uint32_t file_id);

// FDS functions
const char *fds_err_str(ret_code_t ret);
static void fds_evt_handler(fds_evt_t const * p_evt);
static void wait_for_fds_ready(void);

#endif
