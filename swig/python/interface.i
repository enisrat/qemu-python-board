
%module pyboard

#pragma region Typemap Mods */

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

/* ARM regs[16] */
%typemap(in) uint32_t regs[ANY] (uint32_t temp[$1_dim0]) {
  int i;
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim0 elements");
    return NULL;
  }
  for (i = 0; i < $1_dim0; i++) {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyNumber_Check(o)) {
      temp[i] = (uint32_t) PyLong_AsLong(o);
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");      
      return NULL;
    }
  }
  $1 = temp;
}
/* ARM xregs[32] */
%typemap(in) uint64_t xregs[ANY] (uint64_t temp[$1_dim0]) {
  int i;
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim0 elements");
    return NULL;
  }
  for (i = 0; i < $1_dim0; i++) {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyNumber_Check(o)) {
      temp[i] = (uint64_t) PyLong_AsLong(o);
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");      
      return NULL;
    }
  }
  $1 = temp;
}
%typemap(out) uint32_t regs[ANY] {
 int i;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++) {
    PyObject *o = PyLong_FromUnsignedLong((unsigned long) $1[i]);
    PyList_SetItem($result,i,o);
  }
}
%typemap(out) uint64_t xregs[ANY] {
 int i;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++) {
    PyObject *o = PyLong_FromUnsignedLong((unsigned long) $1[i]);
    PyList_SetItem($result,i,o);
  }
}

#pragma endregion Typemap Mods */

#pragma region Header Includes
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
#include "qapi/qapi-types-common.h"
#include "qapi/qapi-types-run-state.h"
//exec
#include "exec/cpu-common.h"
#include "exec/cpu-defs.h"
#include "exec/hwaddr.h"
#include "exec/memattrs.h"
#include "exec/memop.h"
#include "exec/ramlist.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "exec/tb-flush.h"
//block
#include "block/block.h"
#include "block/block-common.h"
#include "block/graph-lock.h"
//sysemu
#include "sysemu/blockdev.h"
#include "sysemu/block-backend-io.h"
#include "sysemu/runstate.h"
#include "sysemu/cpus.h"
//hw
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-core.h"
#include "hw/hotplug.h"
#include "hw/resettable.h"
#include "hw/i2c/i2c.h"
#include "hw/sd/sd.h"
#include "hw/registerfields.h"
#include "hw/core/cpu.h"
%}
#ifdef TARGET_NAME_ARM
	%{
    #include "hw/arm/boot.h"
	#include "hw/arm/allwinner-a10.h"
    #include "hw/timer/allwinner-a10-pit.h"
    #include "hw/intc/allwinner-a10-pic.h"
    #include "hw/net/allwinner_emac.h"
    #include "hw/sd/allwinner-sdhost.h"
    #include "hw/rtc/allwinner-rtc.h"
    #include "hw/misc/allwinner-a10-ccm.h"
    #include "hw/misc/allwinner-a10-dramc.h"
    #include "hw/i2c/allwinner-i2c.h"
    #include "hw/watchdog/allwinner-wdt.h"
    #include "cpu-qom.h"
    #include "cpu.h"
	%}
#endif
#pragma endregion Header Includes

#pragma region some macro removals
#define G_GNUC_WARN_UNUSED_RESULT
#define G_GNUC_NULL_TERMINATED
#define G_GNUC_UNUSED
#define G_DEFINE_AUTOPTR_CLEANUP_FUNC(a,b)
#define G_NORETURN
#define G_GNUC_PRINTF(a,b)
#define __attribute__(x)
#define _Static_assert(a,b)
#define __thread
#pragma endregion

#pragma region Renames and Ignores
#pragma region ignore functions with va_list for now
%ignore object_new_with_propv;
%ignore object_set_propv;
%ignore object_initialize_child_with_propsv;
%ignore error_vprepend;
%ignore error_vprintf;
%ignore error_vreport;
%ignore warn_vreport;
%ignore info_vreport;
#pragma endregion

//fix "property" members in structs --> rename "_property"
%rename("_%s",  "match$ismember"="1") "property";   //swig bug, property is a python keyword
%rename("$ignore",  regextarget=1, fullname=1) "CPUState.*jmp_env"; // does not work because it is of typedef ... x[1]
%ignore pred_esz_masks;

%rename("$ignore",  regextarget=1, fullname=1) "MachineClass.*init";
#pragma endregion Renames and Ignores

#pragma region Include files to wrap */
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
%include "qapi/qapi-types-common.h"
%include "qapi/qapi-types-run-state.h"
//exec
%include "exec/cpu-common.h"
%include "exec/hwaddr.h"
%include "exec/memattrs.h"
%include "exec/memop.h"
%include "exec/ramlist.h"
%include "memory.h.processed.h" // for "exec/memory.h"
%include "exec/address-spaces.h"
%include "exec/tb-flush.h"
//block
%include "block/block.h"
%include "block/block-common.h"
%include "block/graph-lock.h"
//sysemu
%include "sysemu/blockdev.h"
%include "sysemu/block-backend-io.h"
%include "sysemu/runstate.h"
%include "sysemu/cpus.h"
//hw
%include "hw/boards.h"
%include "hw/qdev-properties.h"
%include "hw/qdev-core.h"
%include "hw/hotplug.h"
%include "hw/resettable.h"
%include "hw/i2c/i2c.h"
%include "hw/sd/sd.h"
%include "hw/registerfields.h"
%include "hw/core/cpu.h"

//manual includes to wrap
#pragma region gdbstub/internals.h
// If we want to set Breakpoints of Type BP_GDB, we need to initialize the GDB stub (a little bit),
// otherwise we get SEGFAULTs
%inline %{
    typedef struct GDBState { bool init; } GDBState;
    extern GDBState gdbserver_state;
    void gdb_init_gdbserver_state(void);
    void gdb_create_default_process(GDBState *s);
%}
#pragma endregion
#pragma region qemu/main-loop.h
%inline %{
void main_loop_poll_add_notifier(Notifier *notify);
%}
#pragma endregion
#pragma region qemu/accel.h
#pragma region glib/garray.h
%inline %{
struct GByteArray {
  guint8* data;
  guint len;
};
GByteArray *g_byte_array_sized_new (guint reserved_size);
GByteArray *g_byte_array_set_size (GByteArray *array, guint length);
%}
#pragma endregion
%inline %{
    int accel_supported_gdbstub_sstep_flags(void);
%}
#pragma endregion

#ifdef TARGET_NAME_ARM
    %include "hw/arm/boot.h"
    %import "hw/timer/allwinner-a10-pit.h"
    %import "hw/intc/allwinner-a10-pic.h"
    %import "hw/net/allwinner_emac.h"
    %import "hw/sd/allwinner-sdhost.h"
    %import "hw/rtc/allwinner-rtc.h"
    %import "hw/misc/allwinner-a10-ccm.h"
    %import "hw/misc/allwinner-a10-dramc.h"
    %import "hw/i2c/allwinner-i2c.h"
    %import "hw/watchdog/allwinner-wdt.h"
    %include "hw/arm/allwinner-a10.h"
    %import "cpu-qom.h"
    %include "cpu.h"
#endif
#pragma endregion Include files to wrap */

#pragma region Callback translate functions */

    #pragma region MachineClass
%{ static PyObject *MachineClass_init_cbpyfunc = NULL; 
	static void MachineClass_init_cb(MachineState *ms) {
		if(!MachineClass_init_cbpyfunc) return;
		PyObject * pyobj = SWIG_NewPointerObj(SWIG_as_voidptr(ms), SWIGTYPE_p_MachineState,  0 );
		Py_XDECREF( PyObject_CallFunction(MachineClass_init_cbpyfunc, "(O)", pyobj) ); 
	}
%}
%inline %{
	PyObject *MachineClassInitSet(PyObject *self, PyObject *cb) {
        MachineClass_init_cbpyfunc = cb;
        
        void *argp1;
        int res1 = SWIG_ConvertPtr(self, &argp1,SWIGTYPE_p_MachineClass, 0 |  0 );
        struct MachineClass * arg1 = (struct MachineClass *)(argp1);
        arg1->init = &MachineClass_init_cb;
        return Py_BuildValue("");
    }
%}

%extend MachineClass{
      %pythoncode %{
         init = property(None, _pyboard.MachineClassInitSet)
      %}
}
    #pragma endregion

    #pragma region STATE CHANGE (Breakpoints)
%{
void vm_change_state_handler_cb(void *opaque, bool running, RunState state) {
    PyObject * pyfunc = (PyObject *)opaque;
    Py_XDECREF( PyObject_CallFunction(pyfunc, "(OI)", running ? Py_True: Py_False, state) ); 
}
%}
%inline %{
    void qemu_add_vm_change_state_handler_py(PyObject *cb) {
        qemu_add_vm_change_state_handler(vm_change_state_handler_cb, cb);
    }
%}
//MAIN LOOP RETURN CALLBACK (from main-loop.h)
%{
static PyObject *pynotifier_cb;
void main_loop_notifier_cb(Notifier *notifier, void *data) {
    Py_XDECREF( PyObject_CallFunction(pynotifier_cb, "()") ); 
}
static Notifier pynotifier = {.notify = main_loop_notifier_cb};
%}
%inline %{
void main_loop_poll_add_notifier_py(PyObject *cb) {
    pynotifier_cb = cb;
    main_loop_poll_add_notifier(&pynotifier);
}
%}
    #pragma endregion

    #pragma region MemoryRegionOps Callbacks
    
    %inline %{
    uint64_t MemoryRegionOpsReadCB(void *opaque, hwaddr addr, unsigned size) {
        PyObject *pyops = (PyObject *) opaque;
        PyObject *pyfunc = PyObject_GetAttrString(pyops, "read");

        PyObject *retpy = PyObject_CallFunction(pyfunc, "KK", addr, size);
        uint64_t ret = PyLong_FromUnsignedLong(retpy);
        Py_XDECREF(retpy);

        return ret;        
    }
    void MemoryRegionOpsWriteCB(void *opaque, hwaddr addr, uint64_t data, unsigned size) {
        PyObject *pyops = (PyObject *) opaque;
        PyObject *pyfunc = PyObject_GetAttrString(pyops, "write");

        Py_XDECREF( PyObject_CallFunction(pyfunc, "KKK", addr, data, size) );        
    }
    void MemoryRegionOpsInitForPython(MemoryRegionOps *ops) {
        ops->read = MemoryRegionOpsReadCB;
        ops->write = MemoryRegionOpsWriteCB;
    }
    %}
    %feature("pythonappend") MemoryRegionOps::MemoryRegionOps() %{
        MemoryRegionOpsInitForPython(self)
    %}

    #pragma endregion

#pragma endregion Callback translate functions */

#pragma region Additional helpers */

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

    // Convert PyObject* to "SWIG type void*" to pass around in "opaque" void * args
    void *ToVoidPtr(PyObject *o) {
        return (void *)o;
    }

    //make pointer cast functions, e.g.: Object * ToObject(...)
    //in C you would use explicit cast, in python you can't!
    #define PTRCAST_NAME(type,funcname) type * funcname( void *p) { return (type *) p; };
    #define PTRCAST(type) PTRCAST_NAME(type, To##type);
    PTRCAST(Object)
    PTRCAST(ObjectClass)
    PTRCAST(CPUState)
    PTRCAST(CPUBreakpoint)
    PTRCAST(ARMCPU)

    #ifdef TARGET_NAME_ARM
	    //PTRCAST(AwA10State)
    #endif
%}

#pragma region GetPC function accessible from Python
%inline %{
vaddr GetPC(CPUState *cpu) {
    return cpu->cc->get_pc(cpu);
}
%}
%extend CPUState{
      %pythoncode %{
         pc = property( _pyboard.GetPC, None)

         def _getbplist(self):
            return QTAILQPy(self.breakpoints, "entry")
         bplist = property(_getbplist, None)
        %}
}
#pragma endregion
#pragma endregion Additional helpers */


#pragma region High Level Helpers in PYTHON */

#pragma region wrapping QTAILQ
%pythonbegin %{
class QTAILQPy():
    def __init__(self, head, entry):
        self._h = head
        self.entry = entry
    def __getitem__(self, i):
        try:
            e = self._h.tqh_first
            while i>0:
                e = getattr(e, self.entry).tqe_next
                i -=1
        except:
            raise IndexError()
            return None
        if not e:
            raise IndexError()
            return None
        return e

    def __len__(self):
        i = 0
        e = self._h.tqh_first
        while e:
            i += 1
            e = getattr(e, self.entry).tqe_next
        return i
%}
#pragma endregion
%pythoncode %{
ERR = _GetNullErrorPtr()
def CHECK_ERR():
    e = _CheckError(ERR)
    if e:
        return error_get_pretty(e)
    else:
        return None


#CPU access list
CPUs = QTAILQPy(cvar.cpus_queue, "node")

#pragma region BREAKPOINT HANDLING
AllBPs = {}

def GDBstubInit():
#we need to init gdserver stub to be ableto use BP_GDB breakpoints
    if not cvar.gdbserver_state.init:
        gdb_init_gdbserver_state()
        gdb_create_default_process(cvar.gdbserver_state)

def Breakpoint(cpu, addr, cb):
    GDBstubInit()
    cpu_breakpoint_insert(cpu, addr, BP_GDB, None)
    if cpu.cpu_index not in AllBPs:
        AllBPs[cpu.cpu_index] = {}
    AllBPs[cpu.cpu_index][addr] = cb

def DelBreakpoint(cpu, addr):
    try:
        del AllBPs[cpu.cpu_index][addr]
    except:
        pass


Resume = False
def BPStateChanged(running, state):
    global Resume
    try:
        #import pdb; pdb.set_trace()
        if state == RUN_STATE_DEBUG:
            for cs in CPUs:
                cpu_single_step(cs, 0)
                pc = cs.pc
                if pc in AllBPs[cs.cpu_index]:
                    AllBPs[cs.cpu_index][pc](cs)
                    #tb_flush(cs)
                    if pc == cs.pc: # if we are still at the BP, single step (like GDB does)
                        #print("SINGLESTEP")
                        cpu_single_step(cs, (SSTEP_ENABLE | SSTEP_NOIRQ | SSTEP_NOTIMER) & accel_supported_gdbstub_sstep_flags())
            Resume = True

    except:
        import traceback; traceback.print_exc()

qemu_add_vm_change_state_handler_py(BPStateChanged)

MainLoopCtr = 0
def main_loop_poll_notify():
    global MainLoopCtr, Resume
    #print(f"MAIN LOOP NOTIFIED ME {MainLoopCtr}")
    MainLoopCtr += 1
    if Resume:
        Resume = False
        #print("Resuming")
        vm_start()

main_loop_poll_add_notifier_py(main_loop_poll_notify)
#pragma endregion BREAKPOINT HANDLING
%}
#pragma endregion High Level Helpers */
