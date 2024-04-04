
%module pyboard

/* ######## START Typemap Mods */

/* This is a CHEAT. SWIG does not understand QEMU "qom" Object inheritance. So we deactivate any runtime type checks on pointers here, for very simple python code.
 * @ToDo: Make type safe
*/
%typemap(in) SWIGTYPE * {
  SwigPyObject *sobj = SWIG_Python_GetSwigThis($input);
  if(sobj != NULL)
    $1 = sobj->ptr;
  else
    $1 = NULL;
}
/* ######## END Typemap Mods */

%{
#include <stdbool.h>
//qemu
#include "qemu/osdep.h"
#include "qemu/coroutine-tls.h"
#include "qemu/error-report.h"
#include "qemu/queue.h"
#include "qemu/bswap.h"
#include "qemu/int128.h"
#include "qemu/notify.h"
#include "qemu/rcu.h"
#include "qemu/atomic.h"
#include "qemu/bitmap.h"
#include "qemu/rcu_queue.h"
#include "qemu/clang-tsa.h"
//qom
#include "qom/object.h"
//qapi
#include "qapi/error.h"
//exec
#include "exec/cpu-common.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "exec/memop.h"
#include "exec/ramlist.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
//block
#include "block/block.h"
#include "block/block-common.h"
#include "block/graph-lock.h"
//sysemu
#include "sysemu/blockdev.h"
#include "sysemu/block-backend-io.h"
//hw
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-core.h"
#include "hw/hotplug.h"
#include "hw/resettable.h"
#include "hw/i2c/i2c.h"
#include "hw/sd/sd.h"
%}
#ifdef TARGET_NAME_ARM
	%{
    #include "hw/arm/boot.h"
	#include "hw/arm/allwinner-a10.h"
	%}
#endif



// some macro removals
#define G_GNUC_WARN_UNUSED_RESULT
#define G_GNUC_NULL_TERMINATED
#define G_GNUC_UNUSED
#define G_DEFINE_AUTOPTR_CLEANUP_FUNC(a,b)
#define G_NORETURN
#define G_GNUC_PRINTF(a,b)
#define __attribute__(x)

//ignore functions with va_list for now
%ignore object_new_with_propv;
%ignore object_set_propv;
%ignore object_initialize_child_with_propsv;
%ignore error_vprepend;
%ignore error_vprintf;
%ignore error_vreport;
%ignore warn_vreport;
%ignore info_vreport;


/* ######## START Callback translate functions */

%{ static PyObject *MachineClass_init_cbpyfunc = NULL; 
	static void MachineClass_init_cb(MachineState *ms) {
		if(!MachineClass_init_cbpyfunc) return;
		PyObject * pyobj = SWIG_NewPointerObj(SWIG_as_voidptr(ms), SWIGTYPE_p_MachineState,  0 );
		Py_XDECREF( PyObject_CallFunction(MachineClass_init_cbpyfunc, "(O)", pyobj) ); 
	}
%}
%inline %{
	PyObject *MachineClass_init_set(PyObject *self, PyObject *cb) {
        MachineClass_init_cbpyfunc = cb;
        
        void *argp1;
        int res1 = SWIG_ConvertPtr(self, &argp1,SWIGTYPE_p_MachineClass, 0 |  0 );
        struct MachineClass * arg1 = (struct MachineClass *)(argp1);
        arg1->init = &MachineClass_init_cb;
        return Py_None;
    }
%}
%rename("_init",  regextarget=1, fullname=1) "MachineClass.*init";
%extend MachineClass{
      %pythoncode %{
         init = property(None, _pyboard.MachineClass_init_set)
      %}
}
/* ######## END Callback translate functions */


/* ######## START Miscellaneous fixes */

//fix "property" members in structs --> rename "_property"
%rename("_%s",  "match$ismember"="1") "property";
/* ######## END Miscellaneous fixes */


/* ######## START Include files to wrap */
%include <stdint.i>
//qemu
%import "qemu/compiler.h"
%import "qemu/osdep.h"
%include "qemu/coroutine-tls.h"
%include "qemu/error-report.h"
%include "qemu/queue.h"
    //%include "qemu/bswap.h"
%include "qemu/int128.h"
%include "qemu/notify.h"
%include "qemu/rcu.h"
    //%include "qemu/atomic.h"
%include "qemu/bitmap.h"
%include "qemu/rcu_queue.h"
%import "qemu/clang-tsa.h"
//qom
%include "qom/object.h"
//qapi
%include "qapi/error.h"
//exec
%include "exec/cpu-common.h"
%include "exec/hwaddr.h"
%include "exec/memattrs.h"
%include "exec/memop.h"
%include "exec/ramlist.h"
%include "memory.h.processed.h" // for "exec/memory.h"
%include "exec/address-spaces.h"
//block
%include "block/block.h"
%include "block/block-common.h"
%include "block/graph-lock.h"
//sysemu
%include "sysemu/blockdev.h"
%include "sysemu/block-backend-io.h"
//hw
%include "hw/boards.h"
%include "hw/qdev-properties.h"
%include "hw/qdev-core.h"
%include "hw/hotplug.h"
%include "hw/resettable.h"
%include "hw/i2c/i2c.h"
%include "hw/sd/sd.h"

#ifdef TARGET_NAME_ARM
    %include "hw/arm/boot.h"
	%include "hw/arm/allwinner-a10.h"
    %include "hw/timer/allwinner-a10-pit.h"
    %include "hw/intc/allwinner-a10-pic.h"
    %include "hw/net/allwinner_emac.h"
    %include "hw/sd/allwinner-sdhost.h"
    %include "hw/rtc/allwinner-rtc.h"
    %include "hw/misc/allwinner-a10-ccm.h"
    %include "hw/misc/allwinner-a10-dramc.h"
    %include "hw/i2c/allwinner-i2c.h"
    %include "hw/watchdog/allwinner-wdt.h"
#endif
/* ######## END Include files to wrap */

/* ######## START Additional helpers */

%{
    // for the very first "callback" into python world
    PyObject *MachineClass2Py(MachineClass *mc) {
        PyObject * resultobj = SWIG_NewPointerObj(SWIG_as_voidptr(mc), SWIGTYPE_p_MachineClass, 0 |  0 );
        return resultobj;
    }


%}

%inline %{
    // get an unitialized **Error
    Error **_GetNullErrorPtr(void) {
        return calloc(1, sizeof(Error *));
    };
    // If NULL, no error
    Error *_CheckError(Error **err) {
        return *err;
    };

    //make pointer cast functions, e.g.: Object * ToObject(...)
    //in C you would use explicit cast, in python you can't
    #define PTRCAST_NAME(type,funcname) type * funcname( void *p) { return (type *) p; };
    #define PTRCAST(type) PTRCAST(type, To##type);

    PTRCAST_NAME(Object, OBJ)
    PTRCAST_NAME(ObjectClass, CLASS)

    #ifdef TARGET_NAME_ARM
	    //PTRCAST(AwA10State)
    #endif
%}
/* ######## END Additional helpers */

/* ######## START High Level Helpers */
%pythoncode %{
ERR = _GetNullErrorPtr()
def CHECK_ERR():
    e = _CheckError(ERR)
    if e:
        return error_get_pretty(e)
    else:
        return None
%}
/* ######## END High Level Helpers */
