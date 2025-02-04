#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/units.h"
#include "hw/boards.h"
#include "exec/memory.h"
#include "target/arm/cpu-qom.h"
#include "target/arm/gtimer.h"
#include "hw/arm/bsa.h"
#include "exec/address-spaces.h"
#include "hw/core/cpu.h"
#include "qapi/qmp/qlist.h"
#include "target/arm/cpu.h"
#include "tcg/instrument.h"
#include "qemu/log.h"

static void xpu_programming(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    qemu_log_mask(LOG_TRACE, "XPU init\n");
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void xpu_stuff(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    qemu_log_mask(LOG_TRACE, "XPU STUFF: %llx %llx %llx\n", cpu->env.xregs[0], cpu->env.xregs[1], cpu->env.xregs[2]);
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

void xbl_sec_instrument()
{
    add_instrument(0x148F35B4, -1, xpu_programming, NULL);
    add_instrument(0x148EA520, -1, xpu_stuff, NULL);
    add_instrument(0x148EA4AC, -1, xpu_stuff, NULL);
}