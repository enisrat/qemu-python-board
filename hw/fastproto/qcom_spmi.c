/**
 * Skeleton of a SysBusDevice that compiles.
 * It allows for convenient and fast prototyping using Copy&Paste, Find&Replace + Coding LLMs
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include "qom/object.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "chardev/char-fe.h"

#define TYPE_qcom_spmi "qcom_spmi"
OBJECT_DECLARE_SIMPLE_TYPE(qcom_spmiState, qcom_spmi)

#define NUM_GPIO_OUT 1
#define NUM_GPIO_IN 1

struct qcom_spmiState {
    SysBusDevice parent_obj;
    /* MMIO Regions */
    MemoryRegion mmio1; //config
    MemoryRegion mmio2; //core
    MemoryRegion mmio3; //intr
    /* GPIO out (irq) */
    qemu_irq out[NUM_GPIO_OUT];
    /* GPIO in (this is a bitfield of input PIN states)*/
    uint64_t level_in;
    /* Properties */
    CharBackend prop_chr;
    char *prop_str;
    uint64_t prop_uint64;
    bool prop_bool;

    /* Put your NOT SAVED members here */

    void* _vmstate_saved_offset;
    /* members below this point are SAVED in the vmstate */

};

static Property qcom_spmi_properties[] = {
    DEFINE_PROP_CHR("prop_chr", qcom_spmiState, prop_chr),
    DEFINE_PROP_STRING("prop_str", qcom_spmiState, prop_str),
    DEFINE_PROP_UINT64("prop_uint64", qcom_spmiState, prop_uint64, 0),
    DEFINE_PROP_BOOL("prop_bool", qcom_spmiState, prop_bool, 0),
    DEFINE_PROP_END_OF_LIST(),
};


static uint64_t qcom_spmi_mmio1_read (void *opaque, hwaddr addr, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    uint64_t ret = 0;

    switch (addr) {
    case 0x10:
        ret = 0;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void qcom_spmi_mmio1_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps qcom_spmi_mmio1_ops = {
    .read = qcom_spmi_mmio1_read,
    .write = qcom_spmi_mmio1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static uint64_t qcom_spmi_mmio2_read (void *opaque, hwaddr addr, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    uint64_t ret = 0;

    switch (addr) {
    case 0x10:
        ret = 0;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void qcom_spmi_mmio2_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps qcom_spmi_mmio2_ops = {
    .read = qcom_spmi_mmio2_read,
    .write = qcom_spmi_mmio2_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static uint64_t qcom_spmi_mmio3_read (void *opaque, hwaddr addr, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    uint64_t ret = 0;

    switch (addr) {
    case 0x10:
        ret = 0;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void qcom_spmi_mmio3_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps qcom_spmi_mmio3_ops = {
    .read = qcom_spmi_mmio3_read,
    .write = qcom_spmi_mmio3_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/**
 * Handle input pins. Automatically sets the bits to "level_in" of qcom_spmiState
 */
static void qcom_spmi_gpio_set(void *opaque, int line, int level)
{
    qcom_spmiState *s = (qcom_spmiState *) opaque;
    assert(line >= 0 && line < NUM_GPIO_IN);

    if (level)
        s->level_in |= 1 << line;
    else
        s->level_in &= ~(1 << line);
}

/**
 * VMState automatically saves all direct struct members of qcom_spmiState below _vmstate_saved_offset.
 * If you need to save custom objects, just add them before VMSTATE_END_OF_LIST()
 */
static const VMStateDescription vmstate_qcom_spmi = {
    .name = "qcom_spmi",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        {
        .name = "buf", .version_id = 1,.field_exists = 0,.size = sizeof(qcom_spmiState)-offsetof(qcom_spmiState, _vmstate_saved_offset), 
        .info = &vmstate_info_buffer,.flags=VMS_BUFFER,.offset = offsetof(qcom_spmiState, _vmstate_saved_offset),  
        },
        VMSTATE_END_OF_LIST()
    }
};

static void qcom_spmi_reset(DeviceState *dev)
{
    qcom_spmiState *s = (qcom_spmiState *) dev;
}

static void qcom_spmi_init(Object *obj)
{
    qcom_spmiState *s = (qcom_spmiState *) obj;
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio1, obj, &qcom_spmi_mmio1_ops, s, "qcom_spmi_mmio1", 0x26000);
    sysbus_init_mmio(sbd, &s->mmio1);
    memory_region_init_io(&s->mmio2, obj, &qcom_spmi_mmio2_ops, s, "qcom_spmi_mmio2", 0x1100);
    sysbus_init_mmio(sbd, &s->mmio2);
    memory_region_init_io(&s->mmio3, obj, &qcom_spmi_mmio3_ops, s, "qcom_spmi_mmio3", 0xa0000);
    sysbus_init_mmio(sbd, &s->mmio3);

    for (int i = 0; i < NUM_GPIO_OUT; i++) { sysbus_init_irq(sbd, &s->out[i]); }
    qdev_init_gpio_in_named(dev, qcom_spmi_gpio_set, "in", NUM_GPIO_IN);
}

static void qcom_spmi_realize(DeviceState *dev, Error **errp)
{
    qcom_spmiState *s = (qcom_spmiState *) dev;

}

static void qcom_spmi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_qcom_spmi;
    dc->realize = &qcom_spmi_realize;
    dc->reset = &qcom_spmi_reset;
    device_class_set_props(dc, qcom_spmi_properties);
}

static const TypeInfo qcom_spmi_info = {
    .name          = TYPE_qcom_spmi,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(qcom_spmiState),
    .instance_init = qcom_spmi_init,
    .class_init    = qcom_spmi_class_init,
};

static void qcom_spmi_register_types(void)
{
    type_register_static(&qcom_spmi_info);
}

type_init(qcom_spmi_register_types)
