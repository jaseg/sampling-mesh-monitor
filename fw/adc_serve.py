#!/usr/bin/env python3

import logging
from pathlib import Path
import base64
import threading

import numpy as np
from flask import Flask, render_template, url_for
from flask_socketio import SocketIO
import inotify.adapters, inotify.constants

app = Flask(__name__)
sock = SocketIO(app)
app.logger.setLevel(logging.INFO)

@app.route('/')
def index():
    return render_template('index.html')

def watch_adc_graphs(sock):
    def handler():
        Path('/tmp/adc1_readings.bin').touch()
        Path('/tmp/adc2_readings.bin').touch()

        in_handle = inotify.adapters.Inotify()
        in_handle.add_watch('/tmp/adc2_readings.bin', mask=inotify.constants.IN_CLOSE_WRITE)
        for event in in_handle.event_gen(yield_nones=False):
            app.logger.info('graph update')
            adc1 = np.frombuffer(Path('/tmp/adc1_readings.bin').read_bytes(), dtype=np.uint32)
            adc2 = np.frombuffer(Path('/tmp/adc2_readings.bin').read_bytes(), dtype=np.uint32)
            scale = 3.3 / (2**12) / 128
            adc1, adc2 = adc1 * scale, adc2 * scale
            sock.emit('graph_update', {'adc1': adc1.tolist(),
                                       'adc2': adc2.tolist()})

    adc_thr = threading.Thread(target=handler, daemon=True)
    adc_thr.start()


if __name__ == '__main__':
    watch_adc_graphs(sock)
    sock.run(app)

