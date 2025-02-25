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

#define TYPE_qcom_pimem_ramblur "qcom_pimem_ramblur"
OBJECT_DECLARE_SIMPLE_TYPE(qcom_pimem_ramblurState, qcom_pimem_ramblur)

#define NUM_GPIO_OUT 1
#define NUM_GPIO_IN 1

struct qcom_pimem_ramblurState {
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
    char *prop_char;
    uint64_t prop_uint64;
    bool prop_bool;

    /* Put your NOT SAVED members here */

    char _vmstate_saved_offset;
    /* members below this point are SAVED in the vmstate */
    uint32_t state[0x1100/4];
};

static Property qcom_pimem_ramblur_properties[] = {
    DEFINE_PROP_STRING("prop_char", qcom_pimem_ramblurState, prop_char),
    DEFINE_PROP_UINT64("prop_uint64", qcom_pimem_ramblurState, prop_uint64, 0),
    DEFINE_PROP_BOOL("prop_bool", qcom_pimem_ramblurState, prop_bool, 0),
    DEFINE_PROP_END_OF_LIST(),
};

/**

def read(addr, sz):
	global state
	print(f"{__name__} read {hex(addr)} {hex(sz)}")

	# THere seem to be at most 3 of these "ranges"
	state[0x98] = ( state.get(0x98,0) +1)%2 	#toggle bit
	state[0x9c] = ( state.get(0x9c,0) +1)%2 	#toggle bit
	state[0xa0] = ( state.get(0xa0,0) +1)%2 	#toggle bit
	return state.get(addr, 0)*/
static uint64_t qcom_pimem_ramblur_mmio1_read (void *opaque, hwaddr addr, unsigned size) {
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) opaque;
    uint64_t ret;

    if( addr == 0x98 || addr == 0x9c || addr == 0xa0 ) {
        s->state[addr >> 2] += 1;
        s->state[addr >> 2] %= 2;   //toggle bit
    }

    switch (addr) {
    case 0 ... 0x1100:
        ret = s->state[addr >> 2];
        break;
    default:
        ret = 0;
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void qcom_pimem_ramblur_mmio1_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {
    case 0 ... 0x1100:
       s->state[addr>>2] = value;
    default:
        break;
    }
}
static const MemoryRegionOps qcom_pimem_ramblur_ops = {
    .read = qcom_pimem_ramblur_mmio1_read,
    .write = qcom_pimem_ramblur_mmio1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/**
 * Handle input pins. Automatically sets the bits to "level_in" of qcom_pimem_ramblurState
 */
static void qcom_pimem_ramblur_gpio_set(void *opaque, int line, int level)
{
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) opaque;
    assert(line >= 0 && line < NUM_GPIO_IN);

    if (level)
        s->level_in |= 1 << line;
    else
        s->level_in &= ~(1 << line);
}

/**
 * VMState automatically saves all direct struct members of qcom_pimem_ramblurState below _vmstate_saved_offset.
 * If you need to save custom objects, just add them before VMSTATE_END_OF_LIST()
 */
static const VMStateDescription vmstate_qcom_pimem_ramblur = {
    .name = "qcom_pimem_ramblur",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        {
        .name = "buf", .version_id = 1,.field_exists = 0,.size = sizeof(qcom_pimem_ramblurState)-offsetof(qcom_pimem_ramblurState, _vmstate_saved_offset), 
        .info = &vmstate_info_buffer,.flags=VMS_BUFFER,.offset = offsetof(qcom_pimem_ramblurState, _vmstate_saved_offset),  
        },
        VMSTATE_END_OF_LIST()
    }
};

static void qcom_pimem_ramblur_reset(DeviceState *dev)
{
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) dev;
}

static void qcom_pimem_ramblur_init(Object *obj)
{
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) obj;
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio1, obj, &qcom_pimem_ramblur_ops, s, "qcom_pimem_ramblur_mmio1", 0x8000);
    sysbus_init_mmio(sbd, &s->mmio1);

    for (int i = 0; i < NUM_GPIO_OUT; i++) { sysbus_init_irq(sbd, &s->out[i]); }
    qdev_init_gpio_in_named(dev, qcom_pimem_ramblur_gpio_set, "in", NUM_GPIO_IN);
}

static void qcom_pimem_ramblur_realize(DeviceState *dev, Error **errp)
{
    qcom_pimem_ramblurState *s = (qcom_pimem_ramblurState *) dev;

    s->state[0x30 >> 2] = 0;
    s->state[0x34 >> 2] = 0x80C01000;
}

static void qcom_pimem_ramblur_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_qcom_pimem_ramblur;
    dc->realize = &qcom_pimem_ramblur_realize;
    dc->reset = &qcom_pimem_ramblur_reset;
    device_class_set_props(dc, qcom_pimem_ramblur_properties);
}

static const TypeInfo qcom_pimem_ramblur_info = {
    .name          = TYPE_qcom_pimem_ramblur,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(qcom_pimem_ramblurState),
    .instance_init = qcom_pimem_ramblur_init,
    .class_init    = qcom_pimem_ramblur_class_init,
};

static void qcom_pimem_ramblur_register_types(void)
{
    type_register_static(&qcom_pimem_ramblur_info);
}

type_init(qcom_pimem_ramblur_register_types)
