
#include <unistd.h>
#include "qemu/osdep.h"
#include "hw/core/cpu.h"
#include "exec/cpu-common.h"
#include "exec/coverage.h"
#include "tcg/coverage-tcg.h"

#define HELPER_H  "tcg/coverage-tcg-helper.h"
#include "exec/helper-info.c.inc"
#undef  HELPER_H

#if defined(__x86_64__) || defined(__i386__)
 #include <x86intrin.h>
#endif

static inline uint32_t crc32_i64(uint64_t in) {
#if (defined(__x86_64__) || defined(__i386__))
 	return (uint32_t)_mm_crc32_u64(0, in);
#endif
}

static inline int get_same_bytes_i64(uint64_t a1, uint64_t a2) {
	uint64_t cmp = a1 ^ a2;

	int ctr = 0;
	for(int i=0; i<8; i++) {
		ctr += ((cmp & 0xff) == 0);
		cmp >>= 8;
	}
	return ctr;
}

/**
 * @brief Get index for the CMP Coverage hitmap from the CPU state and the JIT-compiletime pc_diff.
 * 
 */
static inline uint32_t get_index_i64(CPUState *cpu, uint64_t pc_diff) {
	return crc32_i64( cpu->cc->get_pc(cpu) + pc_diff ) & cpu->neg.coverage_rec.comp_rec.mask;
}

/**
 * @brief Helper to record equal byte values of 64-bit "compare" (or SUB) operations.
 * 
 */
void HELPER(record_cmp_i64_u8)(CPUArchState *env, uint64_t pc_diff, uint64_t a1, uint64_t a2)
{
    CPUState *cpu = env_cpu(env);

	((uint8_t*)cpu->neg.coverage_rec.comp_rec.rec_buf_hitmap)[ get_index_i64(cpu, pc_diff) ] += (uint8_t)get_same_bytes_i64(a1, a2);
}

void HELPER(record_cmp_i64_u16)(CPUArchState *env, uint64_t pc_diff, uint64_t a1, uint64_t a2)
{
    CPUState *cpu = env_cpu(env);

	((uint16_t*)cpu->neg.coverage_rec.comp_rec.rec_buf_hitmap)[ get_index_i64(cpu, pc_diff) ] += (uint16_t)get_same_bytes_i64(a1, a2);
}

void HELPER(record_cmp_i64_u32)(CPUArchState *env, uint64_t pc_diff, uint64_t a1, uint64_t a2)
{
	CPUState *cpu = env_cpu(env);

	((uint32_t*)cpu->neg.coverage_rec.comp_rec.rec_buf_hitmap)[ get_index_i64(cpu, pc_diff) ] += (uint32_t)get_same_bytes_i64(a1, a2);
}