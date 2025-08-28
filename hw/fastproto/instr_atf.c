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
#include "crypto/hash.h"
#include "qemu/log.h"


static void retN(CPUState *cs, vaddr pc, void *opaque)
{
    qemu_log_mask(LOG_TRACE, "HIT instrument @%llx cpu %d %llx\n", pc, cs->cpu_index, opaque);
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = (uint64_t)opaque;
    cpu->env.pc = cpu->env.xregs[30];
}

// Helper function to set X0 to 0
static void setX0_0(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = 0;
}

// SPSR for N-EL1 (our custom code)
static void set_SPSR(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = 0b111000100; //Set ERET to AAARCH64 EL1t !
}

// sets the console handlers
static void set_console(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = 0;
    cpu->env.xregs[1] = 0;// we set putchar func to 0 so we get all output via UART!
    cpu->env.xregs[2] = 0;
}

// ATF wants to disable UART_BASE, we do not allow it...
static void set_uart_base(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.pc = cpu->env.pc+4;
}

void atf_teei_instrument()
{
    add_instrument(0x4CE030A4, -1, set_SPSR, 0); //EL3 -> LK
    add_instrument(0x4CE0B370, -1, set_SPSR, 0); //EL3 -> KERNEL
    add_instrument(0x4CE190F0, -1, set_console, 0);
    add_instrument(0x4CE18B84, -1, set_uart_base, 0);
}