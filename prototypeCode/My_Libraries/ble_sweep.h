/*
Author: Thirawat Bureetes, Henry Silva
Email: tbureete@purdue.edu, silva67@purdue.edu
Date: 4/20/2021
Description: A header file contains fucntions and parameters for transfering sweep file via ble.
*/

#include <stdint.h>
#include <string.h>
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"

#ifdef BLE_DEV
#include "nordic_common.h"
#include "nrf.h"
#include "bsp_btn_ble.h"
#include "app_uart.h"
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "sweep.h"


#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "EMI_BLE_DEV"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION 								3000 																		  	/**< The advertising time-out (in units of 10 ms). When set to 0, we will never time out. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define BLE_CON_DEAD					0
#define BLE_CON_ALIVE					1

#define BLE_TRANSFER_IN_PROGRESS		1
#define BLE_TRANSFER_COMPLETE 			0

#ifdef BLE_DEV
#define DUMMY_SWEEP_SIZE                500                                        
#endif

typedef struct package_info
{
	uint8_t *ptr;
	uint16_t start_freq;
	uint16_t stop_freq;
	uint16_t package_size;
} PackageInfo;

void ble_sweep_init(void);
void send_meta_data_ble(MetaData *meta_data);
bool ble_stage_sweep(uint32_t *freq, uint16_t *real, uint16_t *imag, MetaData *meta);
void ble_unstage_sweep(void);
void send_package_ble(uint8_t *package, uint16_t package_size);
PackageInfo pack_sweep_data(uint16_t start_freq, MetaData *meta_data, uint32_t *freq, uint16_t *real, uint16_t *imag);
uint8_t ble_check_connection(void);
uint8_t ble_check_command(void);
bool ble_is_advertising(void);
uint8_t ble_command_handler(void);
void ble_advertise_begin(void);

