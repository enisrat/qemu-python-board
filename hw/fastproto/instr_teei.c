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

static void set_UART_flag(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] |= 0x200;
}

static void set_UART_flag_l4(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] |= 1;
}

void teei_instrument()
{
    add_instrument(0xFFF007354 , -1, set_UART_flag, 0); //Debug output --> UART
    add_instrument(0xFFFFFF80F00144FC, -1, set_UART_flag_l4, 0); //Debug output --> UART
}