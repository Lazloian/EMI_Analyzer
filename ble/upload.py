'''
Author: Thirawat Bureetes
Email: tbureete@purdue.edu
Date: 5/27/2021
Description: This is function for uploading file to the server.
'''

import requests
import yaml
from pathlib import Path, PosixPath
from random import choice
from meta import MetaData

def upload_sweep(sweep: PosixPath, uri: str, meta: MetaData):
    '''
        Upload sweep to the server. Pass a path to sweep file and destination url.
        Return post request response.
    '''
    body = {
        'device_name': meta.device_name,
        'hub_time': meta.hub_timestamp,
        'rssi': meta.rssi,
        'temperature': meta.temperature,
        'sensor_time': meta.time,
        'mac_address': meta.mac_addres
    }

    with open(sweep, 'rb') as f:
        files = {'file': f}
        r = requests.post(uri, files=files, data=body)
        return r

if __name__ == '__main__':

    with open('config.yaml') as f:
        configs = yaml.load(f, Loader=yaml.FullLoader)
    
    path = Path(configs['sweep_path'])
    sweeps = []
    for x in path.iterdir():
        sweeps.append(x)
    sweep = choice(sweeps)
    uri = configs['upload_uri'][configs['env']]
    print(f'Uploading sweep file to {uri}')
    r = upload_sweep(sweep, uri)
    print(f'Status code = {r.status_code}')
    print(r.text)
    