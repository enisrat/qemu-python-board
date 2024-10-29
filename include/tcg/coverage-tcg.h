#ifndef COVERAGE_TCG_H
#define COVERAGE_TCG_H

#include "tcg/tcg.h"
#include "tcg/coverage-tcg-helper-gen.h"

static inline void gen_helper_record_cmp_i64(TCGv_ptr env, TCGv_i64 pc_diff, TCGv_i64 a1, TCGv_i64 a2) {
	if( comp_coverage_record_enabled ) {
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
void tcg_gen_fast_hash_i64(TCGv_i32 dst, TCGv_i64 src, TCGv_i64 src2);

/**
 * Record an edge at the current @pc BB and the id of the outgoing edge.
 * @out_edge_id can be for example 0|1 or the next pc (indirect jump).
 * 
 * Both input parameters are hashed together.
 */
void tcg_gen_rec_edge_i32(TCGv_i32 pc, TCGv_i32 out_edge_id, bool discard_pc);
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id, bool discard_pc);


/**
 * Record matching bytes in COMPARE instructions to guide Fuzzer. NOT USED, generates too much code, use HELPER instead.
 */
void tcg_gen_rec_cmp_i32(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc);
void tcg_gen_rec_cmp_i64(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc);

#endif /* COVERAGE_TCG_H */