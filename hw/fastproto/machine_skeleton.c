
/**
 * Machine (Board) skeleton that compiles. 
 * Includes convenience example code for easy replacement and extension
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/units.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/loader.h"
#include "qemu/datadir.h"
#include "exec/memory.h"

#define CPU_NAME "the_cpu_type"

DeviceState *create_ic()
{
}

static void machinexyz_init(MachineState *machine)
{
    Error *err = NULL;
    Object *o;
    unsigned int smp_cpus = machine->smp.cpus;

    // initialize CPUs
    for (int n = 0; n < smp_cpus; n++)
    {
        Object *cpuobj = object_new(machine->cpu_type);
        object_property_add_child(machine, "cpu[*]", cpuobj);
        qdev_realize(cpuobj, NULL, &error_fatal);
        object_unref(cpuobj);
    }

    // Interrupt Controller (IC) created first
    DeviceState *icdev = create_ic();

    // init devA @0xaaa0000 and connect output pin to IC pin 5
    sysbus_create_varargs("devA", 0xaaa0000, qdev_get_gpio_in(icdev, 5));
    // init devB @0xbbb0000 and connect output pins to IC pins 6,7
    sysbus_create_varargs("devB", 0xbbb0000, qdev_get_gpio_in(icdev, 6), qdev_get_gpio_in(icdev, 7));

    // init devC with two MMIOs @0xccc0000 and @0xddd0000
    o = qdev_new("devC");
    sysbus_realize_and_unref(SYS_BUS_DEVICE(o), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 0, 0xccc0000);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 1, 0xddd0000);
    // connect output pin 0 to IC input pin 8
    sysbus_connect_irq(o, 0, qdev_get_gpio_in(icdev, 8));
    // connect serial 0 to devC
    qdev_prop_set_chr(o, "prop_chr", serial_hd(0));

    // add SRAM @0x14680000 of size 0x40000
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    memory_region_init_ram(sram, 0, "sram", 0x40000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x14680000, sram);

    // add DRAM @0x80000000
    memory_region_add_subregion(get_system_memory(), 0x80000000, machine->ram);

    // load firmware image @0xfff88000
    if (machine->firmware != NULL) {
        char *fn = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
        if (fn != NULL) {
            if (load_image_targphys(fn, 0xfff88000, 0x8000) < 0) {
                error_report("Unable to load %s", machine->firmware);
                exit(1);
            }
            g_free(fn);
        } else {
            error_report("Unable to find %s", machine->firmware);
            exit(1);
        }
    }
    
}

static void machinexyz_machine_init(MachineClass *mc)
{
    static const char *const valid_cpu_types[] = {
        CPU_NAME,
        NULL};

    mc->desc = "machinexyz";
    mc->default_cpu_type = CPU_NAME;
    mc->valid_cpu_types = valid_cpu_types;
    mc->max_cpus = 8;
    mc->default_ram_size = 1 * GiB;
    mc->init = machinexyz_init;
    mc->block_default_type = IF_IDE;
    mc->units_per_default_bus = 1;
    mc->ignore_memory_transaction_failures = true;
    mc->default_ram_id = "machinexyz.ram";
}

DEFINE_MACHINE("machinexyz", machinexyz_machine_init)
