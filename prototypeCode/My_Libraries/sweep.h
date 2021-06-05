/*
 *  sweep.h
 *  
 *  Header file that contains the sweep struct
 *
 *  Author: Henry Silva
 *
 */
 
#ifndef INC_SWEEP_H_
#define INC_SWEEP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// struct to hold sweep metadata
typedef struct sweepData
{
	uint32_t time;
	uint16_t temp;
	uint32_t numPoints;
} MetaData;

// struct to hold sweep parameters
typedef struct sweepParams
{
  // sweep parameters
  uint32_t start;           // the start frequency
  uint32_t delta;           // the size of each increment
  uint16_t steps;           // the number of increments
  uint16_t cycles;          // the number of settling cycle times
  uint8_t cyclesMultiplier; // the multiplier for the settling cycle times
  uint8_t range;            // the output excitation voltage range
  uint8_t clockSource;      // the source of the AD599's system clock
  uint32_t clockFrequency;  // the frequency of the clock for the AD5933
  uint8_t gain;          // the PGA gain of the input frequency

  // sweep information
  uint16_t currentStep;      // the current step the sweep is on
  uint32_t currentFrequency; // the current frequency of the sweep
	
	// sweep metaData
	MetaData metadata;
} Sweep;

// struct to hold sensor config
typedef struct configFile
{
  uint32_t num_sweeps;   // the number of sweeps saved to flash
  uint16_t num_deleted; // the number of sweeps that have been deleted
  uint16_t num_sent;   // the number of sweeps that have been sent over BLE
	uint16_t device_id; // the random id assigned to the device

  Sweep sweep;       // saved sweep paramenters
} Config;
#endif
