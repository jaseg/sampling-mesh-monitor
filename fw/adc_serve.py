#!/usr/bin/env python3

import logging
from pathlib import Path
import base64
import threading
import struct

import numpy as np
from flask import Flask, render_template, url_for
from flask_socketio import SocketIO
import inotify.adapters, inotify.constants

app = Flask(__name__)
sock = SocketIO(app)
app.logger.setLevel(logging.INFO)

dbg_settings = {
        'mesh_flip': False,
        'mesh_flip_n': True,
        'rf_term': False,
        'rf_term_short': False,
        'sampling_phase_var': 2,
        'sampling_width': 2,
        'stimulus_offset': 64,
        'oversampling': 8,
        }

@app.route('/')
def index():
    return render_template('index.html')

def watch_adc_graphs(sock):
    def handler():
        Path('/tmp/adc1_readings.bin').touch()
        Path('/tmp/adc2_readings.bin').touch()
        Path('/tmp/adc1_variance.bin').touch()
        Path('/tmp/adc2_variance.bin').touch()

        in_handle = inotify.adapters.Inotify()
        in_handle.add_watch('/tmp/adc2_variance.bin', mask=inotify.constants.IN_CLOSE_WRITE)
        for event in in_handle.event_gen(yield_nones=False):
            app.logger.info('graph update')
            scale = 3.3 / (2**12) / int(dbg_settings['oversampling'])

            def read_data(fn):
                data = np.frombuffer(Path('/tmp/adc1_readings.bin').read_bytes(), dtype=np.uint32)
                data = data[::-1] # restore causality ;)
                data *= scale
                return data.tolist()
            
            sock.emit('graph_update', {'adc1': read_data('/tmp/adc1_readings.bin'),
                                       'var1': read_data('/tmp/adc1_variance.bin'),
                                       'adc2': read_data('/tmp/adc2_readings.bin'),
                                       'var2': read_data('/tmp/adc2_variance.bin')})

    adc_thr = threading.Thread(target=handler, daemon=True)
    adc_thr.start()

@sock.on('dev_settings')
def handle(data):
    dbg_settings.update(data)
    out = 0
    for i, key in enumerate(['mesh_flip_n', 'mesh_flip', 'rf_term', 'rf_term_short']):
        if dbg_settings[key]:
            out |= 1<<i
    out |= (int(dbg_settings['sampling_phase_var']) & 0xff) << 4
    out |= (int(dbg_settings['sampling_width']) & 0xff) << 12
    out |= (int(dbg_settings['stimulus_offset']) & 0xff) << 20
    out1 = (int(dbg_settings['oversampling']) & 0x1ff)
    print(f'Set debug settings to {out:08x} | {out1:08x}')
    Path('/tmp/dbg_settings.bin').write_bytes(struct.pack('<II', out, out1))

if __name__ == '__main__':
    watch_adc_graphs(sock)
    sock.run(app)

