/**
 * Helper macros for "template" device. We use metalang99 macros.
 */
#include <metalang99.h>

#define DEVNAME_STR         ML99_EVAL( ML99_stringify(v(DEVNAME)) )

#define MMIO(base, size) ML99_tuple(v(base), v(size))
//#define MMIO_LIST ML99_list( MMIO(base, 0x1000), MMIO(area2, 0x1000) )  //empty = ML99_nil()

#define PROP(name, macrotype, type, default) ML99_tuple(v(name), v(macrotype), v(type), v(default))
//#define PROP_LIST ML99_list( PROP(mysize, UINT32, uint32_t, 42), PROP(myflag, BOOL, bool, 1) )  //empty = ML99_nil()

#define PROP_NAME(i) ML99_tupleGet(0)(ML99_listGet(v(i), PROP_LIST)) 
#define PROP_MACROTYPE(i) ML99_tupleGet(1)(ML99_listGet(v(i), PROP_LIST)) 
#define PROP_TYPE(i) ML99_tupleGet(2)(ML99_listGet(v(i), PROP_LIST)) 
#define PROP_DEFAULT(i) ML99_tupleGet(3)(ML99_listGet(v(i), PROP_LIST)) 

#define MMIO_NAME(i) ML99_tupleGet(0)(ML99_listGet(v(i), MMIO_LIST)) 
#define MMIO_SIZE(i) ML99_tupleGet(1)(ML99_listGet(v(i), MMIO_LIST)) 

//#define MMIO_REGION_NAME(i) ML99_cat(v(iomem_), MMIO_NAME(i) ) //not used anymore, MMIO_NAME = region member name

#define MMIO_FUNCNAME_READ(i) ML99_EVAL( ML99_cat4(v(DEVNAME), v(_), MMIO_NAME(i), v(_read) ) )
#define MMIO_FUNCNAME_WRITE(i) ML99_EVAL( ML99_cat4(v(DEVNAME), v(_), MMIO_NAME(i), v(_write) ) )

#define STATE ML99_EVAL( ML99_cat( v(DEVNAME), v(State) ))

#define VMSTATE ML99_EVAL( ML99_cat( v(vmstate_), v(DEVNAME)) )

#define _DEF_MEMREG_IMPL(a) v(MemoryRegion a;)
#define _DEF_MEMREG_ARITY 1

#define _DEF_PROP_MEMBER(name, type) type name; 
#define _DEF_PROP_IMPL(a,i) ML99_invoke(v(_DEF_PROP_MEMBER), v(a), PROP_TYPE(i))
#define _DEF_PROP_ARITY 2

#define _DEF_PROP_ST_IMPL(t,i) ML99_invoke(ML99_cat(v(DEFINE_PROP_), v(t)), ML99_stringify(PROP_NAME(i)), ML99_cat( v(DEVNAME), v(State) ), PROP_NAME(i), PROP_DEFAULT(i) ), v(, )
#define _DEF_PROP_ST_ARITY 2