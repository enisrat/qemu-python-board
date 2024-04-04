
%module pyboard

%{
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