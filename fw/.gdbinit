
target remote localhost:3333
set print pretty on
set print elements 512
set pagination off

# Update GDB's Python paths with the `sys.path` values of the local Python installation,
#  whether that is brew'ed Python, a virtualenv, or another system python.

# Convert GDB to interpret in Python
python
import os,subprocess,sys
# Execute a Python using the user's shell and pull out the sys.path (for site-packages)
paths = subprocess.check_output('python -c "import os,sys;print(os.linesep.join(sys.path).strip())"',shell=True).decode("utf-8").split()
# Extend GDB's Python's search path
sys.path.extend(paths)
end

source upstream/PyCortexMDebug/cmdebug/svd_gdb.py
svd_load upstream/stm32square/svd/STM32G474.svd

break debug_plot_hook
commands
    dump binary memory /tmp/adc_stats.bin &host_data &host_data+1
    restore /tmp/dbg_settings.bin binary dbg_settings
    p/x dbg_settings
    cont
end

load
dump binary memory /tmp/dbg_settings.bin &dbg_settings &dbg_settings+1
cont
