'''
Author: Thirawat Bureetes
Email: tbureete@purdue.edu
Date: 6/02/2021
Description: The main script to connect to ble devices.
'''

import re
import asyncio
import time
import yaml
from bleak import BleakClient
from bleak.backends.device import BLEDevice
from connect import scan_devices, transfer_data
from nordic import save_sweep
from log import logger

def detect_device(scan_duration:int) -> BLEDevice:
    '''
        Scan all devices. Try to detect the proper device. Otherwise, return None.
    '''
    devices = scan_devices(scan_duration)
    for device in devices:
        if re.match(r'(EMI)', device.name):
            return device
    
    return None

async def connect(device: BLEDevice) -> BleakClient:
    '''
        Connect to the BLE device
    '''
    connection = BleakClient(device)
    logger.info(f'Trying to connect to {device.name} ({device.address}) RSSI = {device.rssi}.')
    try:

        await connection.connect()
        while not connection.is_connected:
            await asyncio.sleep(0.5)
        return connection
    
    except TimeoutError: 
        logger.critical('Timeout! Cannot connect to {device.name} ({device.address}).')
        raise asyncio.TimeoutError

    except Exception as e:
        logger.error(f'Unknown error occured when connecting to {device.name} ({device.address}).  Message => {e}')
        raise ConnectionError

async def automate(device: BLEDevice):
    '''
        The main connection and transfer data routine.
    '''
    
    try:
        connection = await asyncio.wait_for(connect(device), timeout=30)
        time.sleep(1)
        if connection is None:
            raise ConnectionError
        logger.info(f'Connected to {device.name} ({device.address}).')
        meta_data, sweep = await asyncio.wait_for(transfer_data(connection), timeout=30)
        meta_data.rssi = device.rssi
        meta_data.device_name = device.name
        meta_data.mac_addres = device.address
        save_sweep(meta_data, sweep)
        if connection.is_connected:
            logger.info(f'Disconnecting from {device.name} ({device.address}).')
            await connection.disconnect()
        

    except asyncio.TimeoutError:
        logger.error('Timeout! Something went wrong.')
    
    except ConnectionError:
        logger.error('Connection was not established. Abort!')

    except Exception as e:
        logger.error(f'Unknown error occured when transfering data from {device.name} ({device.address}). Message => {e}')

            

if __name__ == '__main__':
    with open('config.yaml') as f:
        configs = yaml.load(f, Loader=yaml.FullLoader)

    while True:
        device = detect_device(configs['scan_duration'])
        if device is not None:
            asyncio.run(automate(device))
        else:
            sleep_duration = configs['sleep_duration']
            time.sleep(sleep_duration)
        