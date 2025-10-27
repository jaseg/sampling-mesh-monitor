#!/usr/bin/env python3

import time
import numpy as np
from pathlib import Path

if __name__ == '__main__':
    i = 0
    while True:
        i += 1
        arr1 = np.linspace(0, 2*np.pi, 512)
        arr2 = np.linspace(0, 2*np.pi, 512)
        arr1 = np.sin(arr1 + 2*np.pi*i/20) * 32767 + 32767
        arr2 = np.cos(arr2 + 2*np.pi*i/20) * 32767 + 32767
        Path('/tmp/adc1_readings.bin').write_bytes(arr1.astype(np.uint16).tobytes())
        Path('/tmp/adc2_readings.bin').write_bytes(arr2.astype(np.uint16).tobytes())
        time.sleep(0.1)
