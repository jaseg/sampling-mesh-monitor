#!/usr/bin/env python3

import logging
from pathlib import Path
import base64
import threading
import struct
import json

import numpy as np
from flask import Flask, render_template, url_for
from flask_socketio import SocketIO
import inotify.adapters, inotify.constants

app = Flask(__name__)
sock = SocketIO(app)
app.logger.setLevel(logging.INFO)

dbg_settings = None

@app.route('/')
def index():
    return render_template('index.html')

def watch_adc_graphs(sock, stat_path='/tmp/adc_stats.bin'):
    stat_path = Path(stat_path)

    def handler():
        stat_path.touch()

        in_handle = inotify.adapters.Inotify()
        in_handle.add_watch(str(stat_path), mask=inotify.constants.IN_CLOSE_WRITE)
        for event in in_handle.event_gen(yield_nones=False):
            if dbg_settings is None: # not yet initialized from UI
                print('ignoring update')
                continue

            app.logger.info('graph update')
            oversampling = int(dbg_settings['oversampling']);
            scale = 3.3 / (2**12) / oversampling;

            data = np.frombuffer(stat_path.read_bytes(), dtype=np.uint32)
            # cast to python ints for arbitrary precision arithmetic
            mcu_serial = int(data[0]) | (int(data[1])<<32) | (int(data[2])<<64);
            mcu_serial = f'{mcu_serial:024x}'

            chip_type = {
                    '203632423631500f003f0034': 'pi3hdx12211',
                    '203632423631500f003e002f': 'tdp0604',
                    }.get(mcu_serial, 'unknown')

            data = data[3:].astype(float).reshape((7, 4, -1))
            data[:,:,:] = data[:,:,::-1] # restore causality ;)

            # Scale mean data
            data[:,0,:] *= scale
            data[:,2,:] *= scale

            # Scale variance data
            data[:,1,:] *= scale**2/oversampling
            data[:,3,:] *= scale**2/oversampling

            sock.emit('graph_update', {
                'mcu_serial': mcu_serial,
                'chip_type': chip_type,
                'data': {key: {
                            'adc1': data[idx][0].tolist(),
                            'var1': data[idx][1].tolist(),
                            'adc2': data[idx][2].tolist(),
                            'var2': data[idx][3].tolist(),
                        } for idx, key in enumerate([
                            'cal',
                            'open_fwd',
                            'open_flip',
                            'load_fwd',
                            'load_flip',
                            'short_fwd',
                            'short_flip'])}})

    adc_thr = threading.Thread(target=handler, daemon=True)
    adc_thr.start()

@sock.on('dev_settings')
def handle(data):
    global dbg_settings
    if dbg_settings is None:
        dbg_settings = data
    else:
        dbg_settings.update(data)

    out  = (int(dbg_settings['sampling_phase_var']) & 0xff) << 4
    out |= (int(dbg_settings['sampling_width']) & 0xff) << 12
    out |= (int(dbg_settings['stimulus_offset']) & 0xff) << 20
    out |= (int(bool(dbg_settings['wait_for_trigger'])) & 0x1) << 30
    out1 = (int(dbg_settings['oversampling']) & 0x1ff)
    print(f'Set debug settings to {out:08x} | {out1:08x}')
    Path('/tmp/dbg_settings.bin').write_bytes(struct.pack('<II', out, out1))

if __name__ == '__main__':
    watch_adc_graphs(sock)
    sock.run(app)

