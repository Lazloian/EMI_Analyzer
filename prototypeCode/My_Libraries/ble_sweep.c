/*
Author: Thirawat Bureetes, Henry Silva
Email: tbureete@purdue.edu, silva67@purdue.edu
Date: 4/20/2021
Description: A file contains fucntions and parameters for transfering sweep file via ble.
*/

#include "ble_sweep.h"

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

static MetaData *meta_data_ptr;
static uint32_t *freq_ptr;
static uint16_t *real_ptr, *imag_ptr;
static uint8_t package[BLE_NUS_MAX_DATA_LEN];
static PackageInfo package_info; 
static uint32_t package_sent = 0;
static uint8_t ble_command;
static bool command_received = false;
static bool advertising = false;

/*
This function will check the connection.
*/
uint8_t ble_check_connection(void)
{
	if (m_conn_handle == BLE_CONN_HANDLE_INVALID) 
	{
		return BLE_CON_DEAD;
	}
	else
	{
		return BLE_CON_ALIVE;
	}
}

/*
This function will return if ble is currently advertising
*/
bool ble_is_advertising(void)
{
  return advertising;
}

/*
This function checks if a new command has been received.
It sets command received to false if it was true.
*/
uint8_t ble_check_command(void)
{
  if (command_received)
  {
    command_received = false;
    return ble_command;
  }
  else
  {
    return NULL;
  }
}

/*
This function will handle data transfer. Need to call this function periodically if the connection is alive.
Need to do,  get pointer of metadata, real, imag and freq (separated function). Will impliment asap.
*/
uint8_t ble_command_handler(void)
{
	uint8_t transfer_progress = BLE_TRANSFER_IN_PROGRESS;
	if (ble_check_connection() == BLE_CON_ALIVE && ble_check_command())
	{
		switch (ble_command)
		{
			case 48:
				send_meta_data_ble(meta_data_ptr);
				package_sent = 0;
				transfer_progress = BLE_TRANSFER_IN_PROGRESS;
				break;
			
			case 49:
				package_info = pack_sweep_data(package_sent, meta_data_ptr, freq_ptr, real_ptr, imag_ptr);
				send_package_ble(package_info.ptr, package_info.package_size);
				package_sent = package_info.stop_freq;
				
				package_info = pack_sweep_data(package_sent, meta_data_ptr, freq_ptr, real_ptr, imag_ptr);
				send_package_ble(package_info.ptr, package_info.package_size);
				package_sent = package_info.stop_freq;
			
				package_info = pack_sweep_data(package_sent, meta_data_ptr, freq_ptr, real_ptr, imag_ptr);
				send_package_ble(package_info.ptr, package_info.package_size);
				package_sent = package_info.stop_freq;
			
				NRF_LOG_INFO("Sent frequency upto #%d", package_sent);
			
				if (package_sent <= meta_data_ptr->numPoints)
				{
					transfer_progress = BLE_TRANSFER_IN_PROGRESS;
				}
				else
				{
					transfer_progress = BLE_TRANSFER_COMPLETE;
				}
				break;
		}
	}
	
	return transfer_progress;
	
}

/* This function stages a sweep to be sent over BLE.
 * It sets the pointers in this file to the data to send.
 * IMPORTANT: Sweep must be unstaged when connection is done with unstage_sweep()
 */
bool ble_stage_sweep(uint32_t *freq, uint16_t *real, uint16_t *imag, MetaData *meta)
{ 
  // make sure the pointers are not NULL
  if (freq != NULL && real != NULL && imag != NULL && meta != NULL)
  {
    // set the pointers
    freq_ptr = freq;
    real_ptr = real,
    imag_ptr = imag;
    meta_data_ptr = meta;
		
		NRF_LOG_INFO("Sweeps loaded with %d points", meta->numPoints);
		NRF_LOG_FLUSH();

    return true;
  }
  else
  {
    return false;
  }
}

void ble_unstage_sweep(void)
{
  freq_ptr = NULL;
  real_ptr = NULL;
  imag_ptr = NULL;
  meta_data_ptr = NULL;
}

PackageInfo pack_sweep_data(uint16_t start_freq, MetaData *meta_data, uint32_t *freq, uint16_t *real, uint16_t *imag)
{
	PackageInfo package_info = {
		.ptr = package,
		.start_freq = start_freq,
		.stop_freq = start_freq,
		.package_size = 0
	};
	
	for (uint16_t i=0; i < start_freq; i++)
	{
		freq++;
		real++;
		imag++;
	}
	
	static uint16_t current_size; 
	static uint8_t *data_ptr, *package_ptr;
	
	package_ptr = package_info.ptr;
	package_ptr++;
	for (package_info.package_size=1; package_info.package_size+8 < BLE_NUS_MAX_DATA_LEN && package_info.stop_freq < meta_data->numPoints;)
	{
		data_ptr = (uint8_t *)freq;
		for (current_size=package_info.package_size; package_info.package_size < current_size+4; package_info.package_size++) {
			*package_ptr = *data_ptr;
			data_ptr++;
			package_ptr++;
		}
		
		
		data_ptr = (uint8_t *)real;
		for (current_size=package_info.package_size; package_info.package_size < current_size+2; package_info.package_size++) {
			*package_ptr = *data_ptr;
			data_ptr++;
			package_ptr++;
		}
		
		
		data_ptr = (uint8_t *)imag;
		for (current_size=package_info.package_size; package_info.package_size < current_size+2; package_info.package_size++) {
			*package_ptr = *data_ptr;
			data_ptr++;
			package_ptr++;
		}
		
		freq++;
		real++;
		imag++;
		package_info.stop_freq++;
	}
	*package_info.ptr = package_info.stop_freq - package_info.start_freq;
	return package_info;
}


void send_meta_data_ble(MetaData *meta_data)
{
	
	NRF_LOG_INFO("Sending metadata.");
	NRF_LOG_INFO("The sweep has %d frequency data", meta_data->numPoints);
	static uint8_t buff[11];
	static uint8_t *ptr; 
	uint8_t i = 1;
	buff[0] = 0;
	ptr = (uint8_t *)&meta_data->numPoints;
	for (; i < 5; i++) {
		buff[i] = *ptr;
		ptr++;
	}
	ptr = (uint8_t *)&meta_data->time;
	for (; i < 9; i++) {
		buff[i] = *ptr;
		ptr++;
	}
	ptr = (uint8_t *)&meta_data->temp;
	for (; i < 11; i++) {
		buff[i] = *ptr;
		ptr++;
	}
	
	send_package_ble(buff, (uint16_t)sizeof(buff));
}

/**@brief Function for 
 */
void ble_advertise_begin(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

void send_package_ble(uint8_t *package_to_send, uint16_t size_to_send)
{
		uint32_t err_code;
		do
		{
				err_code = ble_nus_data_send(&m_nus, package_to_send, &size_to_send, m_conn_handle);
				if ((err_code != NRF_ERROR_INVALID_STATE) &&
						(err_code != NRF_ERROR_RESOURCES) &&
						(err_code != NRF_ERROR_NOT_FOUND))
				{
						APP_ERROR_CHECK(err_code);
				}
		} while (err_code == NRF_ERROR_RESOURCES);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
				
//				NRF_LOG_INFO("Recieved: %d", (uint8_t)p_evt->params.rx_data.p_data[0]);
				ble_command = (uint8_t)p_evt->params.rx_data.p_data[0];

        // set command_received to true
        command_received = true;
    }

}


/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the timer module.
 */

static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            advertising = true;
            break;
        case BLE_ADV_EVT_IDLE:
            advertising = false;
            break;
        default:
            break;
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
#ifdef DEBUG_BLE
            NRF_LOG_INFO("Connected");
#endif
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
#ifdef DEBUG_BLE
            NRF_LOG_INFO("Disconnected");
#endif
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            // set command received to false
            command_received = false;
            // unstage the current sweep
            ble_unstage_sweep();
#ifdef DEBUG_BLE
						NRF_LOG_INFO("Stop Advertising");
#endif
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
#ifdef DEBUG_BLE
            NRF_LOG_DEBUG("PHY update request.");
#endif
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
#ifdef DEBUG_BLE
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
#endif
    }
#ifdef DEBUG_BLE
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
#endif
}



/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;
		init.config.ble_adv_on_disconnect_disabled = true;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}




/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


void ble_sweep_init(void)
{
	timers_init();
	ble_stack_init();
	gap_params_init();
	gatt_init();
	services_init();
	advertising_init();
	conn_params_init();
	
#ifdef DEBUG_BLE
	NRF_LOG_INFO("Debug logging for BLE over RTT started.");
#endif
}
