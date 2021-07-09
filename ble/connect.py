'''
Author: Thirawat Bureetes
Email: tbureete@purdue.edu
Date: 5/27/2021
Description: A script for connecting ble.
'''

import asyncio
from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice
from meta import MetaData
from nordic import UUID_NORDIC_RX, UUID_NORDIC_TX, save_sweep


sweep = []
meta_data = MetaData()

def meta_callback(sender, raw_data):
    '''
        Callback funtion for processing raw meta data.
    '''
    message_type = raw_data[0]
    if message_type == 0: # Meta Data
        meta_data.n_freq = int.from_bytes(raw_data[1:5], byteorder='little', signed=False)
        meta_data.time = int.from_bytes(raw_data[5:9], byteorder='little', signed=False)
        meta_data.temperature = int.from_bytes(raw_data[9:11], byteorder='little', signed=False)


def sweep_callback(sender, raw_data):
    '''
        Callback funtion for processing sweep data.
    '''
    message_type = raw_data[0]
    freq_got = int(message_type)
    for i in range(freq_got):
        base_index = i*8
        freq = int.from_bytes(raw_data[base_index+1:base_index+5], byteorder='little', signed=False)
        real = int.from_bytes(raw_data[base_index+5:base_index+7], byteorder='little', signed=True)
        imag = int.from_bytes(raw_data[base_index+7:base_index+9], byteorder='little', signed=True)
        sweep.append({
            'freq': freq, 
            'real': real, 
            'imag': imag
        })


def uart_data_received(sender, raw_data):
    '''
        Callback function for receiving data from sensor.
    '''
    # print(f'RX> ({len(raw_data)} Bytes) {raw_data}')
    message_type = raw_data[0]
    if message_type == 0: # Meta Data
        print(f'Meta Data recieved')
        meta_data.n_freq = int.from_bytes(raw_data[1:5], byteorder='little', signed=False)
        meta_data.time = int.from_bytes(raw_data[5:9], byteorder='little', signed=False)
        meta_data.temperature = int.from_bytes(raw_data[9:11], byteorder='little', signed=False)

    else:
        freq_got = int(message_type)
        print(f'Number of frequency = {freq_got}')
        # print(f'RX> Recieved {len(raw_data)} Bytes')
        for i in range(freq_got):
            base_index = i*8
            freq = int.from_bytes(raw_data[base_index+1:base_index+5], byteorder='little', signed=False)
            real = int.from_bytes(raw_data[base_index+5:base_index+7], byteorder='little', signed=True)
            imag = int.from_bytes(raw_data[base_index+7:base_index+9], byteorder='little', signed=True)
            sweep.append({
                'freq': freq, 
                'real': real, 
                'imag': imag
            })

def connection_command():
    print(f'Select following commands')
    print(f'{"q": >2}: Terminate connection')
    print(f'{"m": >2}: Get metadata')
    print(f'{"g": >2}: Get sweep')
    command = input()
    return command

async def scan(time:int=5):
    '''
        Call Bleak's scaner. It is awaitable function.
    '''
    devices = await BleakScanner.discover(time)
    return devices

def scan_devices(scan_duration:int=5):
    '''
        Acticate Bleak scaner coroutine. Return a list of devices back.
    '''
    devices = asyncio.run(scan(scan_duration))
    return devices

async def connect(device: BLEDevice):
    '''
        Connect to a BLE device then manually transfer data.
    '''
    async with BleakClient(device) as connection:
        while not connection.is_connected:
            await asyncio.sleep(0.5)
        print(f'Connected to {device.address}.')

        meta_data.device_name = device.name
        meta_data.rssi = device.rssi

        await connection.start_notify(UUID_NORDIC_RX, uart_data_received)
        command = connection_command()
        while(command != 'q'):
            if command == 'm':
                await connection.write_gatt_char(UUID_NORDIC_TX, bytearray('0', 'utf-8'), True)
                while meta_data.n_freq <= 0:
                    await asyncio.sleep(0.1)
                print(f'There are {meta_data.n_freq} frequencies')
            elif command == 'g':
                while len(sweep) < meta_data.n_freq:
                    await connection.write_gatt_char(UUID_NORDIC_TX, bytearray('1', 'utf-8'), True)
                    await asyncio.sleep(0.1)
            command = connection_command()

        await asyncio.sleep(0.5)
        print('Disconnecting ...')

async def transfer_data(connection: BleakClient):
    '''
        Automatically transfer data function.
    '''

    sweep.clear()
    meta_data.n_freq = 0

    await connection.start_notify(UUID_NORDIC_RX, meta_callback)

    while meta_data.n_freq <= 0:
        await connection.write_gatt_char(UUID_NORDIC_TX, bytearray('0', 'utf-8'), True)
        await asyncio.sleep(0.5)
    
    await connection.stop_notify(UUID_NORDIC_RX)
    await asyncio.sleep(1)
    await connection.start_notify(UUID_NORDIC_RX, sweep_callback)
        
    while len(sweep) < meta_data.n_freq:
        await connection.write_gatt_char(UUID_NORDIC_TX, bytearray('1', 'utf-8'), True)
        await asyncio.sleep(0.5)

    return meta_data, sweep

def print_devices(devices):
    '''
        Print a list of BLE devices found.
    '''

    print(f'Found {len(devices)} devices')
    print(f' # {"name":>20} {"RSSI":>8} {"address":>20}')
    print('-'*55)
    for index, device in enumerate(devices):
        print(f'{index+1:2} {device.name:>20} {device.rssi:>4} dBm {device.address:>20}')
    print('-'*55)


    
if __name__ == '__main__':

    print('Scaning BLE devices ...')
    devices = scan_devices()
    print_devices(devices)
    selected = int(input('Select a BLE device (0 to rescan): '))
    while selected == 0:
        devices = scan_devices()
        selected = int(input('Select a BLE device (0 to rescan): '))
    device = devices[selected-1]
    print(f'Connecting to {device.name} ({device.address}) ...')

    try:
        asyncio.run(connect(device))
    except Exception as e:
        print('Connection fail.')
        print(e)
        exit(0)
    
    r = save_sweep(meta_data, sweep)
    if r.status_code == 200:
       print('Upload completed.')
    print('Disconnected.')
