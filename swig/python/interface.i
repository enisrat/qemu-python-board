
%module pyboard

%{
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
//#define const
//#define DECLARE_OBJ_CHECKERS(a,b,c,d)
//#define DECLARE_INSTANCE_CHECKER(a,b,c)


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