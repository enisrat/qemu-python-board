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

#define TYPE_qcom_rng "qcom_rng"
OBJECT_DECLARE_SIMPLE_TYPE(qcom_rngState, qcom_rng)

#define NUM_GPIO_OUT 1
#define NUM_GPIO_IN 1

struct qcom_rngState {
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

    void* _vmstate_saved_offset;
    /* members below this point are SAVED in the vmstate */
	uint32_t state[0x140/4];
};

static Property qcom_rng_properties[] = {
    DEFINE_PROP_STRING("prop_char", qcom_rngState, prop_char),
    DEFINE_PROP_UINT64("prop_uint64", qcom_rngState, prop_uint64, 0),
    DEFINE_PROP_BOOL("prop_bool", qcom_rngState, prop_bool, 0),
    DEFINE_PROP_END_OF_LIST(),
};

/**
 * @brief state = [0]*(size//4)
def read(addr, sz):
	print(f"RNG read {hex(addr)} {hex(sz)}")
	return {0: 1, 4: 1, 0x140: 1<<0x19}.get(addr, state[addr])
def write(addr, data, sz):
	state[addr] = data
	print(f"RNG write {hex(addr)} size {hex(sz)} val {hex(data)}")
 */
static uint64_t qcom_rng_mmio1_read (void *opaque, hwaddr addr, unsigned size) {
    qcom_rngState *s = (qcom_rngState *) opaque;
    uint64_t ret;

    switch (addr) {
	case 0x0000:
		ret = 1;
        break;
    case 0x0004:
        return 1;
        break;
    case 0x140:
        ret = 1<<0x19;
        break;
    case 0x144 ... 0x1000: // upper limit for debugging
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        break;
    default:
        return s->state[addr >> 2];
    }

    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, ret);
    return ret;
}
static void qcom_rng_mmio1_write (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    qcom_rngState *s = (qcom_rngState *) opaque;
    qemu_log_mask(LOG_TRACE, "%s: off %"HWADDR_PRIx" sz %u val %"PRIx64"\n", __func__, addr, size, value);

    switch (addr) {
    case 0x144 ... 0x1000:   //upper limit for debugging
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        break;
    default:
        s->state[addr >> 2] = value;
        //qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps qcom_rng_ops = {
    .read = qcom_rng_mmio1_read,
    .write = qcom_rng_mmio1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/**
 * Handle input pins. Automatically sets the bits to "level_in" of qcom_rngState
 */
static void qcom_rng_gpio_set(void *opaque, int line, int level)
{
    qcom_rngState *s = (qcom_rngState *) opaque;
    assert(line >= 0 && line < NUM_GPIO_IN);

    if (level)
        s->level_in |= 1 << line;
    else
        s->level_in &= ~(1 << line);
}

/**
 * VMState automatically saves all direct struct members of qcom_rngState below _vmstate_saved_offset.
 * If you need to save custom objects, just add them before VMSTATE_END_OF_LIST()
 */
static const VMStateDescription vmstate_qcom_rng = {
    .name = "qcom_rng",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        {
        .name = "buf", .version_id = 1,.field_exists = 0,.size = sizeof(qcom_rngState)-offsetof(qcom_rngState, _vmstate_saved_offset), 
        .info = &vmstate_info_buffer,.flags=VMS_BUFFER,.offset = offsetof(qcom_rngState, _vmstate_saved_offset),  
        },
        VMSTATE_END_OF_LIST()
    }
};

static void qcom_rng_reset(DeviceState *dev)
{
    qcom_rngState *s = (qcom_rngState *) dev;
}

static void qcom_rng_init(Object *obj)
{
    qcom_rngState *s = (qcom_rngState *) obj;
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio1, obj, &qcom_rng_ops, s, "qcom_rng_mmio1", 0x1000);
    sysbus_init_mmio(sbd, &s->mmio1);

    for (int i = 0; i < NUM_GPIO_OUT; i++) { sysbus_init_irq(sbd, &s->out[i]); }
    qdev_init_gpio_in_named(dev, qcom_rng_gpio_set, "in", NUM_GPIO_IN);
}

static void qcom_rng_realize(DeviceState *dev, Error **errp)
{
    qcom_rngState *s = (qcom_rngState *) dev;

}

static void qcom_rng_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_qcom_rng;
    dc->realize = &qcom_rng_realize;
    dc->reset = &qcom_rng_reset;
    device_class_set_props(dc, qcom_rng_properties);
}

static const TypeInfo qcom_rng_info = {
    .name          = TYPE_qcom_rng,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(qcom_rngState),
    .instance_init = qcom_rng_init,
    .class_init    = qcom_rng_class_init,
};

static void qcom_rng_register_types(void)
{
    type_register_static(&qcom_rng_info);
}

type_init(qcom_rng_register_types)
