/**
 * Template to "generate" a single SysBusDevice without code duplications.
 * Simply copy this file, rename it "mydevxx.c" and fill in the gaps @FILL
 */

#include "template.h"

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "hw/sysbus.h"

/* @FILL: Unique device name */
#define DEVNAME             bla
#define DEVNAME_UPPERCASE   BLA

/** 
 * @FILL: Define MMIOs. Automatically generates MemoryRegions and read(), write() functions.
 * Syntax: MMIO(base, size) Example: MMIO(base, 0x1000)
 */
#define MMIO_LIST ML99_list( MMIO(base, 0x1000) )  //empty = ML99_nil()

/**
 * @FILL: Define Properties. Automatically generates members and defitinitions.
 * Syntax: PROP(myname, TYPE, type, default_value) Example: PROP(mysize, UINT32, uint32_t, 42)
 * The first TYPE is for the qdev macros, e.g. DEFINE_PROP_UINT32. 
 * The second one (uint32_t) is the C type used for the struct member.
 */
#define PROP_LIST ML99_list( PROP(mysize, UINT32, uint32_t, 42), PROP(myflag, BOOL, bool, 1) )  //empty = ML99_nil()


struct STATE {
    SysBusDevice parent_obj;

    /* MMIO Regions */
    ML99_EVAL( ML99_listUnwrap(ML99_listMap(ML99_compose(v(_DEF_MEMREG),v(ML99_PRIV_tupleGet_0)), MMIO_LIST)) )
    /* Properties */
    ML99_EVAL( ML99_listMapInPlaceI(ML99_compose(v(_DEF_PROP),v(ML99_PRIV_tupleGet_0)), PROP_LIST) )

    /* IRQ (ignore if not needed) */
    qemu_irq irq;

    /* @FILL: members below this point are saved in a VMSTATE_BUFFER */
    char _vmstate_saved_offset;

};
typedef struct STATE STATE;
G_DEFINE_AUTOPTR_CLEANUP_FUNC(STATE, object_unref);

static Property ML99_EVAL( ML99_cat( v(DEVNAME), v(_properties)))[] = {
    ML99_EVAL( ML99_listMapInPlaceI(v(_DEF_PROP_ST), ML99_listMap(v(ML99_PRIV_tupleGet_1), PROP_LIST)) )
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription VMSTATE = {
    .name = DEVNAME_STR,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        {
        .name = "buf", .version_id = 1,.field_exists = 0,.size = sizeof(STATE)-offsetof(STATE, _vmstate_saved_offset), 
        .info = &vmstate_info_buffer,.flags= VMS_BUFFER,.offset = offsetof(STATE, _vmstate_saved_offset),  
        },
        VMSTATE_END_OF_LIST()
    }
};

/**
 * @FILL: The MMIO callbacks to fill in
 */
#if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 0
static uint64_t MMIO_FUNCNAME_READ(0) (void *opaque, hwaddr addr, unsigned size) {
    STATE *s = (STATE *) opaque;
    uint64_t ret;

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }

    return ret;
}
static void MMIO_FUNCNAME_WRITE(0) (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    STATE *s = (STATE *) opaque;

    switch (addr) {

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}
static const MemoryRegionOps ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(0), v(_ops)) ) = {
    .read = MMIO_FUNCNAME_READ(0),
    .write = MMIO_FUNCNAME_WRITE(0),
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};
#endif
#if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 1
static uint64_t MMIO_FUNCNAME_READ(1) (void *opaque, hwaddr addr, unsigned size) {
}
static void MMIO_FUNCNAME_WRITE(1) (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
}
static const MemoryRegionOps ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(1), v(_ops)) ) = {
    .read = MMIO_FUNCNAME_READ(1),
    .write = MMIO_FUNCNAME_WRITE(1),
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};
#endif
#if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 2
static uint64_t MMIO_FUNCNAME_READ(2) (void *opaque, hwaddr addr, unsigned size) {
}
static void MMIO_FUNCNAME_WRITE(2) (void *opaque, hwaddr addr, uint64_t value, unsigned size) {
}
static const MemoryRegionOps ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(2), v(_ops)) ) = {
    .read = MMIO_FUNCNAME_READ(2),
    .write = MMIO_FUNCNAME_WRITE(2),
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};
#endif

static void ML99_EVAL( ML99_cat( v(DEVNAME), v(_init)) )(Object *obj)
{
    STATE *s = (STATE *)obj;
}
static void ML99_EVAL( ML99_cat( v(DEVNAME), v(_realize)) )(DeviceState *dev, Error **errp)
{
    STATE *s = (STATE *)dev;

    #if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 0
    memory_region_init_io(&s->ML99_EVAL(MMIO_NAME(0)), OBJECT(dev), &ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(0), v(_ops)) ), s, DEVNAME_STR, ML99_EVAL( MMIO_SIZE(0) ));
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->ML99_EVAL(MMIO_NAME(0)));
    #endif
    #if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 1
    memory_region_init_io(&s->ML99_EVAL(MMIO_NAME(1)), OBJECT(dev), &ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(1), v(_ops)) ), s, DEVNAME_STR, ML99_EVAL( MMIO_SIZE(1) ));
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->ML99_EVAL(MMIO_NAME(1)));
    #endif
    #if  ML99_EVAL( ML99_listLen(MMIO_LIST) ) > 2
    memory_region_init_io(&s->ML99_EVAL(MMIO_NAME(2)), OBJECT(dev), &ML99_EVAL( ML99_cat4( v(DEVNAME), v(_), MMIO_NAME(2), v(_ops)) ), s, DEVNAME_STR, ML99_EVAL( MMIO_SIZE(2) ));
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->ML99_EVAL(MMIO_NAME(2)));
    #endif

    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);

    /* @FILL: If you need to do additional stuff in device realize() */
}

static void ML99_EVAL( ML99_cat( v(DEVNAME), v(_class_init)) )(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &VMSTATE;
    dc->realize = &ML99_EVAL( ML99_cat( v(DEVNAME), v(_realize)) );
    device_class_set_props(dc, ML99_EVAL( ML99_cat( v(DEVNAME), v(_properties))));
}

static const TypeInfo ML99_EVAL( ML99_cat( v(DEVNAME), v(_info)) ) = {
    .name          = DEVNAME_STR,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STATE),
    .instance_init = ML99_EVAL( ML99_cat( v(DEVNAME), v(_init)) ),
    .class_init    = ML99_EVAL( ML99_cat( v(DEVNAME), v(_class_init)) ),
};

static void ML99_EVAL( ML99_cat( v(DEVNAME), v(_register_types)) )(void)
{
    type_register_static(&ML99_EVAL( ML99_cat( v(DEVNAME), v(_info)) ));
}

type_init(ML99_EVAL( ML99_cat( v(DEVNAME), v(_register_types)) ))
