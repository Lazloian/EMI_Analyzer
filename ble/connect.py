'''
Author: Thirawat Bureetes
Email: tbureete@purdue.edu
Date: 5/27/2021
Description: The main script to connect to ble devices.
'''

from datetime import datetime, timezone
import yaml
from pathlib import Path
from pandas import DataFrame

import asyncio
from bleak import BleakScanner
from bleak import BleakClient

from upload import upload_sweep

UART_NORDIC = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E'
UUID_NORDIC_RX = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E'
UUID_NORDIC_TX = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E'

class MetaData():
    def __init__(self):
        self.n_freq = 0
        self.temperature = 0
        self.time = 0
    
    def __str__(self):
        return f'Number of frequency data = {self.n_freq} \n' \
               f'Time = {self.time} \n' \
               f'temperature = {self.temperature}'

def uart_data_received(sender, raw_data):
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

async def scan(time=5):
    devices = await BleakScanner.discover(time)
    return devices

async def connect(device):
    async with BleakClient(device) as connection:
        while not connection.is_connected:
            pass
        print('Connected')
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
        print('Disconnecting ...')

def scan_devices():
    print('Scaning BLE devices ...')
    devices = asyncio.run(scan())
    print(f'Found {len(devices)} devices')
    print(f' # {"name":>20} {"RSSI":>8} {"address":>20}')
    print('-'*55)
    for index, device in enumerate(devices):
        print(f'{index+1:2} {device.name:>20} {device.rssi:>4} dBm {device.address:>20}')
    print('-'*55)
    return devices

with open('config.yaml') as f:
    configs = yaml.load(f, Loader=yaml.FullLoader)

devices = scan_devices()
selected = int(input('Select a BLE device (0 to rescan): '))
while selected == 0:
    devices = scan_devices()
    selected = int(input('Select a BLE device (0 to rescan): '))
device = devices[selected-1]
print(f'Connecting to {device.name} ({device.address}) ...')

meta_data = MetaData()
sweep = []
try:
    asyncio.run(connect(device))
except:
    print('Connection fail.')
    exit(0)
print(f'Got {len(sweep)} data')
sweep_df = DataFrame.from_dict(sweep)
print(sweep_df)
path = Path('/home/pi/Desktop/EMI/sweeps')
filename = path / f'{device.name}-{datetime.now(timezone.utc).replace(microsecond=0).isoformat()}.csv'
sweep_df.to_csv(filename, index=False)
print(f'Save to {filename}')
uri = configs['upload_uri'][configs['env']]
print(f'Uploading sweep file to {uri}')
r = upload_sweep(filename, uri)
if r.status_code == 200:
   print('Upload completed.')
print('Disconnected.')