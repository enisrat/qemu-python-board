from pyboard import *


def pyboard_init(ms):
    print(f"Hello from pyboard_init: {ms}")

def machine_init(mc):
    print(f"Hello from PYTHON machine_init {mc}")
    print(f"Board Name: {mc.name}")

    #import pdb; pdb.set_trace()

    mc.desc = "Python example as 'cubietech cubieboard (Cortex-A8)'"
    mc.default_cpu_type = "cortex-a8-arm-cpu"
    mc.default_ram_size = 512*1024*1024
    mc.init = pyboard_init;
    mc.block_default_type = IF_IDE;
    mc.units_per_default_bus = 1
    mc.ignore_memory_transaction_failures = True
    mc.default_ram_id = "cubieboard.ram"

