#ifndef COVERAGE_TCG_H
#define COVERAGE_TCG_H

#include "tcg/tcg.h"

/**
 * CISC ADD instruction. Specifically for x86 optimization.
 * Corresponds to: ADD byte|word|dword|qword ptr [base + index*elem_sz + ofs], val
 */
void tcg_gen_add_cisc_i64(TCGv_i64 base, TCGv_i64 index,  TCGv_i64 val, tcg_target_long elem_sz, tcg_target_long ofs);


/**
 * Do a fast hash like CRC32. Used for coverage recording and fuzzing.
 */
void tcg_gen_fast_hash_i32(TCGv_i32 dst, TCGv_i32 src);
void tcg_gen_fast_hash_i64(TCGv_i32 dst, TCGv_i64 src);

/**
 * Record an edge at the current @pc BB and the id of the outgoing edge.
 * @out_edge_id can be for example 0|1 or the next pc (indirect jump).
 * 
 * Both input parameters are hashed together.
 * @discard_pc: pc is discarded right after read/hashing to relieve temp var pressure.
 */
void tcg_gen_rec_edge_i32(TCGv_i32 pc, TCGv_i32 out_edge_id, bool discard_pc);
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id, bool discard_pc);


/**
 * Record matching bytes in COMPARE instructions to guide Fuzzer.
 */
void tcg_gen_rec_cmp_i32(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc);
void tcg_gen_rec_cmp_i64(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc);

#endif /* COVERAGE_TCG_H */