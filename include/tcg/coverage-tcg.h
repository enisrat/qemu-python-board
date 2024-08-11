#ifndef COVERAGE_TCG_H
#define COVERAGE_TCG_H

#include "tcg/tcg.h"


/**
 * Do a fast hash like CRC32. Used for coverage recording and fuzzing.
 */
void tcg_gen_fast_hash_i32(TCGv_i32 dst, TCGv_i32 src);
void tcg_gen_fast_hash_i64(TCGv_i32 dst, TCGv_i64 src);

/**
 * Record an edge at the current @pc and the id of the outgoing edge.
 * @out_edge_id can be for example 0|1 or the next pc (indirect jump).
 * 
 * Both input parameters are hashed together.
 */
void tcg_gen_rec_edge_i32(TCGv_i32 pc, TCGv_i32 out_edge_id);
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id);

#endif /* COVERAGE_TCG_H */