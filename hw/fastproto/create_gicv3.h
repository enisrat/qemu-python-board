/**
 * Generic static helper function to create an ARM GICv3
 */


#include "qemu/osdep.h"
#include "qemu/datadir.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qapi/qmp/qlist.h"
#include "target/arm/cpu-qom.h"
#include "target/arm/gtimer.h"
#include "hw/arm/bsa.h"
#include "hw/intc/arm_gicv3_common.h"
#include "hw/intc/arm_gicv3_its_common.h"
#include "exec/address-spaces.h"

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
         * Mapping from the output timer irq lines from the CPU to the
         * GIC PPI inputs used for this board.
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