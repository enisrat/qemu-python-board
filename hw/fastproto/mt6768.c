/**
 * mt6768
 * 
 * Example QEMU args:
 * ./qemu-system-aarch64 -machine mt6768 ... ??
 * 
 * -object memory-backend-file,id=id,size=size,mem-path=dir,share=on|off,discard-data=on|off,merge=on|off,dump=on|off,prealloc=on|off,host-nodes=host-nodes,policy=default|preferred|bind|interleave,align=align,offset=offset,readonly=on|off,rom=on|off|auto
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
#include "target/arm/cpu-qom.h"
#include "target/arm/gtimer.h"
#include "hw/arm/bsa.h"
#include "hw/intc/arm_gicv3_common.h"
#include "hw/intc/arm_gicv3_its_common.h"
#include "exec/address-spaces.h"
#include "hw/core/cpu.h"
#include "qapi/qmp/qlist.h"
#include "target/arm/cpu.h"
#include "tcg/instrument.h"
#include "crypto/hash.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"
#include "chardev/char.h"
#include "sysemu/hostmem.h"
#include "hw/misc/unimp.h"

#define CPU_NAME "cortex-a55-arm-cpu"


static DeviceState * create_gicv3(int num_irqs, hwaddr dist, hwaddr redist)
{
    unsigned int smp_cpus = MACHINE(qdev_get_machine())->smp.cpus;
    SysBusDevice *gicbusdev;
    const char *gictype;
    uint32_t redist0_count;
    QList *redist_region_count;
    int i;

    gictype = gicv3_class_name();

    DeviceState *gic;
    gic = qdev_new(gictype);
    qdev_prop_set_uint32(gic, "revision", 3);
    qdev_prop_set_uint32(gic, "num-cpu", smp_cpus);
    /*
     * Note that the num-irq property counts both internal and external
     * interrupts; there are always 32 of the former (mandated by GIC spec).
     */
    qdev_prop_set_uint32(gic, "num-irq", num_irqs + 32);
    qdev_prop_set_bit(gic, "has-security-extensions", true);

    // may need adjusting
    redist0_count = smp_cpus;

    redist_region_count = qlist_new();
    qlist_append_int(redist_region_count, redist0_count);
    qdev_prop_set_array(gic, "redist-region-count", redist_region_count);

    object_property_set_link(OBJECT(gic), "sysmem",
                             get_system_memory(), &error_fatal);
    qdev_prop_set_bit(gic, "has-lpi", true);

    gicbusdev = SYS_BUS_DEVICE(gic);
    sysbus_realize_and_unref(gicbusdev, &error_fatal);
    sysbus_mmio_map(gicbusdev, 0, dist);
    sysbus_mmio_map(gicbusdev, 1, redist);

    /*
     * Wire the outputs from each CPU's generic timer and the GICv3
     * maintenance interrupt signal to the appropriate GIC PPI inputs,
     * and the GIC's IRQ/FIQ/VIRQ/VFIQ interrupt outputs to the CPU's inputs.
     */
    for (i = 0; i < smp_cpus; i++) {
        DeviceState *cpudev = DEVICE(qemu_get_cpu(i));
        int intidbase = num_irqs + i * GIC_INTERNAL;
        int irq;
        /*
    """# ARM Architectural Timer Interrupt(GIC PPI) numbers
    gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|17
    gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|18
    gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|0
    gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|19"""
         */
        const int timer_irq[] = {
            [GTIMER_PHYS] = ARCH_TIMER_NS_EL1_IRQ,
            [GTIMER_VIRT] = ARCH_TIMER_VIRT_IRQ,
            [GTIMER_HYP]  = ARCH_TIMER_NS_EL2_IRQ,
            [GTIMER_SEC]  = ARCH_TIMER_S_EL1_IRQ,
            [GTIMER_HYPVIRT] = ARCH_TIMER_NS_EL2_VIRT_IRQ,
        };

        for (irq = 0; irq < ARRAY_SIZE(timer_irq); irq++) {
            qdev_connect_gpio_out(cpudev, irq,
                                  qdev_get_gpio_in(gic,
                                                   intidbase + timer_irq[irq]));
        }

        qdev_connect_gpio_out_named(cpudev, "gicv3-maintenance-interrupt", 0,
                                    qdev_get_gpio_in(gic,
                                                     intidbase
                                                     + ARCH_GIC_MAINT_IRQ));

        qdev_connect_gpio_out_named(cpudev, "pmu-interrupt", 0,
                                    qdev_get_gpio_in(gic,
                                                     intidbase
                                                     + VIRTUAL_PMU_IRQ));

        sysbus_connect_irq(gicbusdev, i, qdev_get_gpio_in(cpudev, ARM_CPU_IRQ));
        sysbus_connect_irq(gicbusdev, i + smp_cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_FIQ));
        sysbus_connect_irq(gicbusdev, i + 2 * smp_cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_VIRQ));
        sysbus_connect_irq(gicbusdev, i + 3 * smp_cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_VFIQ));
    }
    return gic;
}

static DeviceState *create_ic()
{
    return create_gicv3(320, 0x0c000000, 0x0c040000);
}

// pull in modularized code
void atf_teei_instrument();
void teei_instrument();

static void mt6768_init(MachineState * machine)
{
    Error *err = NULL;
    Object *o;
    unsigned int smp_cpus = machine->smp.cpus;

    // initialize CPUs
    for (int n = 0; n < smp_cpus; n++)
    {
        Object *cpuobj = object_new(machine->cpu_type);
        object_property_add_child(machine, "cpu[*]", cpuobj);
        qdev_prop_set_bit(cpuobj, "start-powered-off", n > 0);
        //qdev_prop_set_uint64(cpuobj, "mp_affinity", )
        qdev_realize(cpuobj, NULL, &error_fatal);
        object_unref(cpuobj);
    }

    // Interrupt Controller (IC) created first
    DeviceState *icdev = create_ic();

    sysbus_create_varargs("mtk_mcucfg", 0xC530000, NULL);

    o = qdev_new("mtk_uart");
    Chardev *chr = qemu_chr_find("uart0");
    if(!chr){
        error_report("chardev with id \"uart0\" not found\n");
        exit(1);
    }
    qdev_prop_set_chr(o, "prop_chr", chr);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(o), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 0, 0x11002000);

    // o = qdev_new("qcom_spmi");
    // sysbus_realize_and_unref(SYS_BUS_DEVICE(o), &error_fatal);
    // sysbus_mmio_map(SYS_BUS_DEVICE(o), 0, 0x0c40a000);

    // o = object_resolve_path_component(object_get_objects_root(), "dram");
    // memory_region_add_subregion(get_system_memory(), 0x40000000, &MEMORY_BACKEND(o)->mr);
    MemoryRegion *dram = g_new(MemoryRegion, 1);
    memory_region_init_ram(dram, 0, "dram", 0xC0000000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x40000000, dram);


    MemoryRegion *sram1 = g_new(MemoryRegion, 1);
    memory_region_init_ram(sram1, 0, "sram1", 0x100000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x100000, sram1);

    o = object_resolve_path_component(object_get_objects_root(), "sram2");
    memory_region_add_subregion(get_system_memory(), 0x200000, &MEMORY_BACKEND(o)->mr);

    create_unimplemented_device("a", 0x300000, 0x01002000-0x300000);
    create_unimplemented_device("b", 0x01003000, 0x40000000-0x01003000);
    create_unimplemented_device("c", 0x100000000, 0x100000000);


    if( load_image_targphys("atf", 0x4CE01000, 0x100000) < 0 ){
        error_report("could not load atf");
        exit(1);
    }
    if( load_image_targphys("atf_arg_t", 0x4CE00000, 0x100000) < 0 ){
        error_report("could not load atf_arg_t");
        exit(1);
    }
    if( load_image_targphys("mtk_bl_param_t", 0x4C080000, 0x100000) < 0 ){
        error_report("could not load mtk_bl_param_t");
        exit(1);
    }
    if( load_image_targphys("atags", 0x4C11DA80, 0x100000) < 0 ){
        error_report("could not load atags");
        exit(1);
    }
    if( load_image_targphys("tee", 0x70000000, 0x400000) < 0 ){
        error_report("could not load tee");
        exit(1);
    }
    if( load_image_targphys("lk", 0x4c400000, 0x800000) < 0 ){
        error_report("could not load lk");
        exit(1);
    }

    ARMCPU * cs = qemu_get_cpu(0);

    cpu_reset(cs);
    arm_emulate_firmware_reset(cs, 3);

    cpu_set_pc(cs, 0x4CE01000);
    cs->env.xregs[0] = 0x4C080000;
    cs->env.xregs[1] = 0x0;
    arm_rebuild_hflags(&cs->env);
    init_instrument_htable();

    atf_teei_instrument();
    teei_instrument();
}

static void mt6768_machine_init(MachineClass *mc)
{
    static const char *const valid_cpu_types[] = {
        CPU_NAME,
        NULL};

    mc->desc = "mt6768";
    mc->default_cpu_type = CPU_NAME;
    mc->valid_cpu_types = valid_cpu_types;
    mc->max_cpus = 8;
    mc->default_ram_size = 3 * GiB;
    mc->minimum_page_bits = 12;
    mc->init = mt6768_init;
    mc->block_default_type = IF_IDE;
    mc->units_per_default_bus = 1;
    mc->ignore_memory_transaction_failures = true;
    mc->default_ram_id = "mt6768.ram";
}

DEFINE_MACHINE("mt6768", mt6768_machine_init)
