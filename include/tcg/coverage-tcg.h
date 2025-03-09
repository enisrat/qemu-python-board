#ifndef COVERAGE_TCG_H
#define COVERAGE_TCG_H

#include "tcg/tcg.h"
#include "tcg/coverage-tcg-helper-gen.h"

/* shadow-argument of the struct DisasContext used by the guest code translator.
 * Need this for CMP COVERAGE Recording, because this disas ctx is not always available as a function argument when needed */
extern __thread void *current_disasctx; 

static inline void gen_helper_record_cmp_i64(TCGv_ptr env, TCGv_i64 pc_diff, TCGv_i64 a1, TCGv_i64 a2) {
	if( comp_coverage_record_tcg_enabled ) {
		if( comp_coverage_record_elem_size == 1) {
			gen_helper_record_cmp_i64_u8(env, pc_diff, a1, a2);
		} else if( comp_coverage_record_elem_size == 2) {
			gen_helper_record_cmp_i64_u16(env, pc_diff, a1, a2);
		} else if ( comp_coverage_record_elem_size == 4) {
			gen_helper_record_cmp_i64_u32(env, pc_diff, a1, a2);
		}
	}
}

/**
 * Do a fast hash like CRC32. Used for coverage recording and fuzzing.
 */
void tcg_gen_fast_hash_i32(TCGv_i32 dst, TCGv_i32 src, TCGv_i32 src2);
void tcg_gen_fast_hash_i64(TCGv_i32 dst, TCGv_i32 src, TCGv_i64 src2);

/**
 * Record an edge at the current @pc basic block and the id of the outgoing edge.
 * @out_edge_id can be for example 0|1 or the next pc (indirect jump).
 *
 * @pc and @out_edge_id are not clobbered
 *
 * The way you use this is to insert a call to this in the guest translation routines
 * for conditional branches, once the out_edge_id is known.
 *
 * This will generate coverage hashmap that is quite accurate.
 * Both input parameters are hashed together.
 *
 * For performance reasons, often the @pc will be the _beginning_ address
 * of the translation block, not the address of the conditional branch insn.
 * The reason is that in QEMU, the pc value is not updated in every translated instruction
 * (see DisasContext::pc_save).
 * You have two options :
 * 1. update the PC right before calling this ( gen_xxx_update_pc() )
 * 2. do not update PC. This is mostly fine, because a basic block is well-defined by its
 * _beginning_ address only. But there are rare cases this is not entirely true and can 
 * generate (slightly) unstable coverage hitmaps:
 * When a translated block looks like this:
 * 	op1	<-- PC
 * 	op2
 * 	op3
 * 	call_helper_XYZ()
 *  op4
 *  {tcg_gen_rec_edge_X(PC, COND)}
 *  branch if COND
 *
 * In call_helper_XYZ() it might rarely happen that cpu_loop_exit_restore() is triggered.
 * This results in a break of the translation block. When the CPU continues after that, 
 * a new translation block is generated like this:
 *
 * op4 <-- PC
 * {tcg_gen_rec_edge_X(PC, COND)}
 * branch if COND
 * 
 * As you can see, now the coverage recording will use another PC value.
 * This might happen only rarely and this tradeoff be justified.
 */
void tcg_gen_rec_edge_i32(TCGv_i32 pc, TCGv_i32 out_edge_id);
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i32 out_edge_id);

/**
 * ADD [mem+idx*str+ofs], val instruction. This is declared because it optimizes well for x86.
 * Corresponds to: add byte|word|dword|qword ptr [base + index*elem_sz + ofs], val
 */
void tcg_gen_add_mem_idx_i64(TCGv_i64 base, TCGv_i64 index, TCGv_i64 val, int elem_sz, int ofs);


#endif /* COVERAGE_TCG_H */
