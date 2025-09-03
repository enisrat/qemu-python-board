/**
 * Redfin (Pixel 4a5G / 5)
 * 
 * Example QEMU args:
 * ./qemu-system-aarch64 -machine redfin -smp maxcpus=8 -bios pbl_sdm865.bin -drive file=bootlun.bin,if=none,id=dr7,readonly=on -device ufs-lu,drive=dr7,bus=ufs-bus,lun=7  -drive file=sde,if=none,id=dr0,readonly=on  -device ufs-lu,drive=dr0,bus=ufs-bus,lun=0 -drive file=sda,if=none,id=dr1,readonly=on  -device ufs-lu,drive=dr1,bus=ufs-bus,lun=1 -drive file=sdd,if=none,id=dr2,readonly=on  -device ufs-lu,drive=dr2,bus=ufs-bus,lun=2 -chardev socket,host=localhost,port=9876,id=qup -nographic
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

#define CPU_NAME "cortex-a53-arm-cpu"


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
            [GTIMER_PHYS] = 18,
            [GTIMER_VIRT] = 19,
            [GTIMER_HYP]  = 0,
            [GTIMER_SEC]  = 17,
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
    return create_gicv3(640, 0x17a00000, 0x17a60000);
}

// pull in modularized code
void brom_instrument();
void xbl_sec_instrument();
void sbl1_instrument();
void xbl_uefi_instrument();

static void redfin_init(MachineState * machine)
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
        qdev_realize(cpuobj, NULL, &error_fatal);
        object_unref(cpuobj);
    }

    // Interrupt Controller (IC) created first
    DeviceState *icdev = create_ic();

    sysbus_create_varargs("ufs", 0x1d84000, NULL);

    sysbus_create_varargs("qcom_clkdom", 0x17800000 + 0x00541000, NULL);
    sysbus_create_varargs("qcom_mpm2_sleepctr", 0xC221000, NULL);
    sysbus_create_varargs("qcom_pimem_ramblur", 0x610000, NULL);
    sysbus_create_varargs("qcom_gpll4_mode", 0x177000, NULL);
    sysbus_create_varargs("qcom_prng", 0x791000, NULL);
    sysbus_create_varargs("qcom_qfprom", 0x780000, NULL);
    sysbus_create_varargs("qcom_rng", 0x793000, NULL);
    sysbus_create_varargs("qcom_smmu", 0x15000000, NULL);
    sysbus_create_varargs("qcom_tcsr_boot_misc_detect", 0x1FD3000, NULL);
    sysbus_create_varargs("qcom_tcsr_wonce", 0x1FD4000, NULL);
    sysbus_create_varargs("qcom_tcsr_devconfig", 0x1FC8000, NULL);
    sysbus_create_varargs("qcom_tcsr_mutex", 0x1F40000, NULL);
    sysbus_create_varargs("qcom_timer1", 0x17C21000, NULL);
    sysbus_create_varargs("qcom_ufsphy", 0x1D87000, NULL);
    sysbus_create_varargs("qcom_0x1dc0000", 0x1DC0000, NULL);
    sysbus_create_varargs("qcom_0x90c0000", 0x90c0000, NULL);
    sysbus_create_varargs("qcom_0x189000", 0x189000, NULL);
    sysbus_create_varargs("qcom_0x190000", 0x190000, NULL);
    sysbus_create_varargs("qcom_0xc230000", 0xc230000, NULL);
    sysbus_create_varargs("qcom_qtimer1", 0x17C20000, NULL);
    sysbus_create_varargs("qcom_0xc600000", 0xc600000, NULL);

    o = qdev_new("qcom_qup");
    Chardev *chr = qemu_chr_find("qup");
    if(!chr){
        error_report("chardev with id \"qup\" not found\n");
        exit(1);
    }
    qdev_prop_set_chr(o, "prop_chr", chr);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(o), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 0, 0x888000);

    o = qdev_new("qcom_spmi");
    sysbus_realize_and_unref(SYS_BUS_DEVICE(o), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 0, 0x0c40a000);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 1, 0x0c440000);
    sysbus_mmio_map(SYS_BUS_DEVICE(o), 2, 0x0e700000);

    // add SRAM @0x14680000 of size 0x40000
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    memory_region_init_ram(sram, 0, "sram", 0x40000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x14680000, sram);

    // add SRAM_SEC @0x14800000 of size 0x100000
    MemoryRegion *sram_sec = g_new(MemoryRegion, 1);
    memory_region_init_ram(sram_sec, 0, "sram_sec", 0x100000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x14800000, sram_sec);

    // add PIMEM @0x1c000000 of size 0x04000000
    MemoryRegion *pimem = g_new(MemoryRegion, 1);
    memory_region_init_ram(pimem, 0, "pimem", 0x04000000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x1c000000, pimem);

    // add AOPMEM @0x0b000000 of size 0x100000
    MemoryRegion *aopmem = g_new(MemoryRegion, 1);
    memory_region_init_ram(aopmem, 0, "aopmem", 0x100000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x0b000000, aopmem);

    // add DRAM @0x80000000
    memory_region_add_subregion(get_system_memory(), 0x80000000, machine->ram);

    MemoryRegion *rom = g_new(MemoryRegion, 1);
    memory_region_init_rom(rom, 0, "rom", 0x100000, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x300000, rom);

    // load BOOTROM image @300000
    if (machine->firmware != NULL) {
        char *fn = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
        if (fn != NULL) {
            if (load_image_targphys(fn, 0x300000, 0x100000) < 0) {
                error_report("Unable to load %s", machine->firmware);
                exit(1);
            }
            g_free(fn);
        } else {
            error_report("Unable to find %s", machine->firmware);
            exit(1);
        }
    }

    ARMCPU * cs = qemu_get_cpu(0);

    cpu_reset(cs);
    arm_emulate_firmware_reset(cs, 3);

    cpu_set_pc(cs, 0x300000);
    arm_rebuild_hflags(&cs->env);
    init_instrument_htable();

    brom_instrument();
    xbl_sec_instrument();
    sbl1_instrument();
    tz_instrument();
    xbl_uefi_instrument();
}

static void redfin_machine_init(MachineClass *mc)
{
    static const char *const valid_cpu_types[] = {
        CPU_NAME,
        NULL};

    mc->desc = "redfin";
    mc->default_cpu_type = CPU_NAME;
    mc->valid_cpu_types = valid_cpu_types;
    mc->max_cpus = 8;
    mc->default_ram_size = 1 * GiB;
    mc->minimum_page_bits = 12;
    mc->init = redfin_init;
    mc->block_default_type = IF_IDE;
    mc->units_per_default_bus = 1;
    mc->ignore_memory_transaction_failures = true;
    mc->default_ram_id = "redfin.ram";
}

DEFINE_MACHINE("redfin", redfin_machine_init)
