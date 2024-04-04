from pyboard import *
import sys

err = GetNullErrorPtr()

def pyboard_init(ms):
    print(f"Hello from pyboard_init: {ms}")

    if ms.firmware:
        error_report("BIOS not supported for this machine")
        sys.exit(1)

    if ms.ram_size != 512*1024*1024 and ms.ram_size != 1024*1024*1024:
        error_report("This machine can only be used with 512MiB or 1GiB RAM")
        sys.exit(1)

    if not "cortex-a8" in ms.cpu_type:
        error_report("This board can only be used with cortex-a8 CPU")
        sys.exit(1)

    import pdb;pdb.set_trace()

    a10 = object_new("allwinner-a10")
    object_property_add_child(ToObject(ms), "soc", a10)
    object_unref(a10)


    object_property_set_int(a10.emac, "phy-addr", 1, err)
    object_property_set_int(a10.timer, "clock0-freq", 32768, err)
    object_property_set_int(a10.timer, "clock1-freq", 24000000, err)

    qdev_realize(a10, 0, err)



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

