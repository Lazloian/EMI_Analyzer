'''
Author: Thirawat Bureetes
Email: tbureete@purdue.edu
Date: 5/27/2021
Description: API for manage sweep uploading.
'''

import yaml
from datetime import datetime
from flask_restful  import Resource, reqparse
from werkzeug.utils import secure_filename
from werkzeug.datastructures import FileStorage
from pathlib import Path
from models import db, Sweep, Device



with open('config.yaml') as f:
    configs = yaml.load(f, Loader=yaml.FullLoader)
    path = Path(configs['sweep_path'])

class UploadHandler(Resource):

    def get(self):
        return {'message': 'If you see this, it means the system works!'}

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument('file', type=FileStorage, location='files', required=True)
        parser.add_argument('device_name', required=True)
        parser.add_argument('mac_address', required=True)
        parser.add_argument('hub_time', required=True)
        parser.add_argument('sensor_time')
        parser.add_argument('rssi')
        args = parser.parse_args()
        
        # check if the device is already in the database or not
        device = Device.query.filter_by(name=args['device_name'],
                                        mac_address = args['mac_address']).first()
        if device is None:
            print(f'Device is not found. Creating a new one.')
            device = Device(
                name = args['device_name'],
                mac_address = args['mac_address'],
                timestamp = args['hub_time']
            )
            db.session.add(device)

        else:
            print(f'{device} found.')
            device.last_updated = args['hub_time']
        
        # save file
        upload_file = args['file']
        filename = secure_filename(upload_file.filename)
        upload_file.save(path / filename)
        sweep_meta = Sweep(
            filename = filename,
            hub_time = datetime.fromisoformat(args['hub_time']),
            rssi = args['rssi']
        )
        device.sweeps.append(sweep_meta)
        db.session.add(sweep_meta)

        db.session.commit()
      
        return sweep_meta.json(), 201
