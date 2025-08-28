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

#define TYPE_mtk_mcucfg "mtk_mcucfg"
OBJECT_DECLARE_SIMPLE_TYPE(mtk_mcucfgState, mtk_mcucfg)

#define NUM_GPIO_OUT 1
#define NUM_GPIO_IN 1

struct mtk_mcucfgState {
    SysBusDevice parent_obj;
    /* MMIO Regions */
    MemoryRegion mmio1;
    MemoryRegion mmio2;
    MemoryRegion mmio3;
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

static Property mtk_mcucfg_properties[] = {
    DEFINE_PROP_CHR("prop_chr", mtk_mcucfgState, prop_chr),
    DEFINE_PROP_STRING("prop_str", mtk_mcucfgState, prop_str),
    DEFINE_PROP_UINT64("prop_uint64", mtk_mcucfgState, prop_uint64, 0),
    DEFINE_PROP_BOOL("prop_bool", mtk_mcucfgState, prop_bool, 0),
    DEFINE_PROP_END_OF_LIST(),
};

/**
 * mmio1 definitions
 * If you want more than one MMIO region: 
 * simply duplicate below _read(), _write() and _ops code , then Find&Replace mmio1 -> mmio2 
 * and add memory_region_init_io(...) below
 * */
static uint64_t mtk_mcucfg_mmio1_read (void *opaque, hwaddr addr, unsigned size) {
    mtk_mcucfgState *s = (mtk_mcucfgState *) opaque;
    uint64_t ret = 0;

    switch (addr) {
    case 0xA840:
        ret = 0xC001;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void mtk_mcucfg_mmio1_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    mtk_mcucfgState *s = (mtk_mcucfgState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps mtk_mcucfg_mmio1_ops = {
    .read = mtk_mcucfg_mmio1_read,
    .write = mtk_mcucfg_mmio1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/**
 * Handle input pins. Automatically sets the bits to "level_in" of mtk_mcucfgState
 */
static void mtk_mcucfg_gpio_set(void *opaque, int line, int level)
{
    mtk_mcucfgState *s = (mtk_mcucfgState *) opaque;
    assert(line >= 0 && line < NUM_GPIO_IN);

    if (level)
        s->level_in |= 1 << line;
    else
        s->level_in &= ~(1 << line);
}

/**
 * VMState automatically saves all direct struct members of mtk_mcucfgState below _vmstate_saved_offset.
 * If you need to save custom objects, just add them before VMSTATE_END_OF_LIST()
 */
static const VMStateDescription vmstate_mtk_mcucfg = {
    .name = "mtk_mcucfg",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        {
        .name = "buf", .version_id = 1,.field_exists = 0,.size = sizeof(mtk_mcucfgState)-offsetof(mtk_mcucfgState, _vmstate_saved_offset), 
        .info = &vmstate_info_buffer,.flags=VMS_BUFFER,.offset = offsetof(mtk_mcucfgState, _vmstate_saved_offset),  
        },
        VMSTATE_END_OF_LIST()
    }
};

static void mtk_mcucfg_reset(DeviceState *dev)
{
    mtk_mcucfgState *s = (mtk_mcucfgState *) dev;
}

static void mtk_mcucfg_init(Object *obj)
{
    mtk_mcucfgState *s = (mtk_mcucfgState *) obj;
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio1, obj, &mtk_mcucfg_mmio1_ops, s, "mtk_mcucfg_mmio1", 0x10000);
    sysbus_init_mmio(sbd, &s->mmio1);

    for (int i = 0; i < NUM_GPIO_OUT; i++) { sysbus_init_irq(sbd, &s->out[i]); }
    qdev_init_gpio_in_named(dev, mtk_mcucfg_gpio_set, "in", NUM_GPIO_IN);
}

static void mtk_mcucfg_realize(DeviceState *dev, Error **errp)
{
    mtk_mcucfgState *s = (mtk_mcucfgState *) dev;

}

static void mtk_mcucfg_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_mtk_mcucfg;
    dc->realize = &mtk_mcucfg_realize;
    dc->reset = &mtk_mcucfg_reset;
    device_class_set_props(dc, mtk_mcucfg_properties);
}

static const TypeInfo mtk_mcucfg_info = {
    .name          = TYPE_mtk_mcucfg,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(mtk_mcucfgState),
    .instance_init = mtk_mcucfg_init,
    .class_init    = mtk_mcucfg_class_init,
};

static void mtk_mcucfg_register_types(void)
{
    type_register_static(&mtk_mcucfg_info);
}

type_init(mtk_mcucfg_register_types)
