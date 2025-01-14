/*
 * Support for instrumenting guest code. 
 * Similar to breakpoints, this is invisible to guest and GDB and compiled directly into intermediate code.
 */

#ifndef QEMU_INSTRUMENT_H
#define QEMU_INSTRUMENT_H

#include "qemu/qht.h"
#include "exec/vaddr.h"
#include "hw/core/cpu.h"
#include "stdbool.h"

typedef void (*InstrumentCallback) (CPUState *cs, vaddr pc, void *opaque);

/*
 * Is the PC for a given vCPU really instrumented?
 * Quick check to make while translating
 */
bool check_instrument(vaddr pc, int cpu_index);
bool call_instrument_cb(CPUState *cs, vaddr pc);

/* Add a instrumentation "breakpoint" to be compiled into intermediate TCG
 * cpu_index = -1 matches all vCPUs
 */
bool add_instrument(vaddr pc, int cpu_index, InstrumentCallback cb, void *opaque);

bool remove_instrument(vaddr pc, int cpu_index);

void init_instrument_htable(void);





#endif