
%module pyboard

%{
#include <stdbool.h>
#include "qemu/osdep.h"
#include "qemu/coroutine-tls.h"
#include "qom/object.h"
#include "exec/cpu-common.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "exec/memop.h"
#include "qemu/queue.h"
#include "exec/ramlist.h"
#include "qemu/bswap.h"
#include "qemu/int128.h"
#include "qemu/notify.h"
#include "qemu/rcu.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "block/block.h"
%}


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


/* ######## Callback translate functions */

%{ static PyObject *MachineClass_init_cbpyfunc = NULL; 
	static void MachineClass_init_cb(MachineState *ms) {
		if(!MachineClass_init_cbpyfunc) return;
		PyObject * pyobj = SWIG_NewPointerObj(SWIG_as_voidptr(ms), SWIGTYPE_p_MachineState,  0 );
		Py_XDECREF( PyObject_CallFunction(MachineClass_init_cbpyfunc, "(O)", pyobj) ); 
	}
	
	PyObject *MachineClass_init_set(PyObject *self, PyObject *cb) {
        MachineClass_init_cbpyfunc = cb;
        
        void *argp1;
        int res1 = SWIG_ConvertPtr(self, &argp1,SWIGTYPE_p_MachineClass, 0 |  0 );
        struct MachineClass * arg1 = (struct MachineClass *)(argp1);
        arg1->init = &MachineClass_init_cb;
        return Py_None;
    }
%}
PyObject *MachineClass_init_set(PyObject *self, PyObject *cb);
%rename("_init",  regextarget=1, fullname=1) "MachineClass.*init";
%extend MachineClass{
      %pythoncode %{
         init = property(None, _pyboard.MachineClass_init_set)
      %}
}
/* ######## END Callback translate functions */


%include <stdint.i>
%import "qemu/coroutine-tls.h"
%include "qom/object.h"
%include "exec/cpu-common.h"
%include "exec/hwaddr.h"
%include "exec/memattrs.h"
%include "exec/memop.h"
%include "qemu/queue.h"
%include "exec/ramlist.h"
//%include "qemu/bswap.h"
%include "qemu/int128.h"
%include "qemu/notify.h"
%include "qemu/rcu.h"
%include "qapi/error.h"
%include "qemu/error-report.h"
%include "hw/boards.h"
%include "hw/qdev-properties.h"
%include "sysemu/blockdev.h"


%wrapper {
    // for the very first "callback" into python world
    PyObject *MachineClass2Py(MachineClass *mc) {
        PyObject * resultobj = SWIG_NewPointerObj(SWIG_as_voidptr(mc), SWIGTYPE_p_MachineClass, 0 |  0 );
        return resultobj;
    }
}
