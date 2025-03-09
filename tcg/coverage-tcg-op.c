
#include "qemu/osdep.h"
#include "hw/core/cpu.h"
#include "tcg/tcg.h"
#include "tcg/tcg-temp-internal.h"
#include "tcg/tcg-op-common.h"
#include "tcg-internal.h"
#include "exec/coverage.h"

void tcg_gen_add_mem_idx_i64(TCGv_i64 base, TCGv_i64 index, TCGv_i64 val, int elem_sz, int ofs)
{
    if(TCG_TARGET_HAS_add_mem_idx_i64) {
        tcg_gen_op5(INDEX_op_add_mem_idx_i64, tcgv_i64_arg(base), tcgv_i64_arg(index), tcgv_i64_arg(val), elem_sz, ofs);
    } else {
        tcg_debug_assert(elem_sz == 1 || elem_sz == 2 || elem_sz == 4 || elem_sz == 8);
        TCGv_i64 t0 = tcg_temp_ebb_new_i64();
        TCGv_i64 t1 = tcg_temp_ebb_new_i64();

        if(elem_sz == 1){
            tcg_gen_mov_i64(t0, index);
            tcg_gen_add_i64(t0, t0, base);
            tcg_gen_ld8u_i64(t1, (TCGv_ptr)t0, ofs);
            tcg_gen_add_i64(t1, t1, val);
            tcg_gen_st8_i64(t1, (TCGv_ptr)t0, ofs);
        }if (elem_sz == 2){
            tcg_gen_shli_i64(t0, index, 1);
            tcg_gen_add_i64(t0, t0, base);
            tcg_gen_ld16u_i64(t1, (TCGv_ptr)t0, ofs);
            tcg_gen_add_i64(t1, t1, val);
            tcg_gen_st16_i64(t1, (TCGv_ptr)t0, ofs);
        }if (elem_sz == 4){
            tcg_gen_shli_i64(t0, index, 2);
            tcg_gen_add_i64(t0, t0, base);
            tcg_gen_ld32u_i64(t1, (TCGv_ptr)t0, ofs);
            tcg_gen_add_i64(t1, t1, val);
            tcg_gen_st32_i64(t1, (TCGv_ptr)t0, ofs);
        }if (elem_sz == 8){
            tcg_gen_shli_i64(t0, index, 3);
            tcg_gen_add_i64(t0, t0, base);
            tcg_gen_ld_i64(t1, (TCGv_ptr)t0, ofs);
            tcg_gen_add_i64(t1, t1, val);
            tcg_gen_st_i64(t1, (TCGv_ptr)t0, ofs);
        }

        tcg_temp_free_i64(t0);
        tcg_temp_free_i64(t1);
    }
}

/**
 * Generate minimal code to set:
 * vCPU...edge hitmap[ CRC32(pc|out_edge_id) ] += 1
 */
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i32 out_edge_id) {
    if(edge_coverage_record_tcg_enabled) {
        TCGv_i32 hashed = tcg_temp_new_i32();
        tcg_gen_mov_i32(hashed, out_edge_id);
        tcg_gen_fast_hash_i64(hashed, (TCGv_i64)hashed, pc);

        TCGv_ptr baseptr = tcg_temp_new_ptr();
        tcg_gen_ld_ptr(baseptr, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_rec.rec_buf_hitmap) -
                                            (int) sizeof(CPUNegativeOffsetState)));
        TCGv_i64 mask = tcg_temp_new_i64();
        tcg_gen_ld_i32((TCGv_i32)mask, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_rec.mask) -
                                            (int) sizeof(CPUNegativeOffsetState)));

        tcg_gen_and_i32(hashed, hashed, mask);
        tcg_gen_add_mem_idx_i64((TCGv_i64)baseptr, (TCGv_i64)hashed, tcg_constant_i64(1), edge_coverage_record_elem_size, 0);

        /*
        //liveness pass will take care of that
        tcg_temp_free_i64(hashofs);
        tcg_temp_free_ptr(baseptr);
        tcg_temp_free_i64(mask);*/
    }
}

