from pyboard import *
import sys


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

    #import pdb;pdb.set_trace()

    a10 = AW_A10(object_new("allwinner-a10"))
    object_property_add_child(ms, "soc", a10)
    object_unref(a10)


    object_property_set_int(a10.emac, "phy-addr", 1, ERR)
    object_property_set_int(a10.timer, "clk0-freq", 32768, ERR)
    if CHECK_ERR():
        print(f"Failed to set clock from Python: {CHECK_ERR()}")
    object_property_set_int(a10.timer, "clk1-freq", 24000000, ERR)

    qdev_realize(a10, 0, ERR)
    print("qdev_realize OK")

    #import pdb; pdb.set_trace()

    #Connect AXP 209 */
    i2c = qdev_get_child_bus(a10.i2c0, "i2c")
    i2c_slave_create_simple(i2c, "axp209_pmu", 0x34)

    # Retrieve SD bus */
    di = drive_get(IF_SD, 0, 0);
    if di:
        blk =  blk_by_legacy_dinfo(di)
    else:
        di = 0
    bus = qdev_get_child_bus(DEVICE(a10), "sd-bus")

    # Plug in SD card */
    carddev = qdev_new(TYPE_SD_CARD);
    qdev_prop_set_drive_err(carddev, "drive", blk, ERR)
    qdev_realize_and_unref(carddev, bus, ERR)

    memory_region_add_subregion(get_system_memory(), AW_A10_SDRAM_BASE, ms.ram)

    # Load target kernel or start using BootROM */
    if not ms.kernel_filename and blk and blk_is_available(blk):
        # Use Boot ROM to copy data from SD card to SRAM */
        allwinner_a10_bootrom_setup(a10, blk)
    
    # TODO create and connect IDE devices for ide_drive_get() */

    binfo = arm_boot_info()

    binfo.ram_size = ms.ram_size
    arm_load_kernel(a10.cpu, ms, binfo)

    print("arm_load_kernel OK")



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

