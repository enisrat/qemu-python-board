
#include "qemu/osdep.h"
#include "hw/core/cpu.h"
#include "tcg/tcg.h"
#include "tcg/tcg-temp-internal.h"
#include "tcg/tcg-op-common.h"
#include "tcg-internal.h"
#include "exec/coverage.h"


/**
 * CISC ADD instruction. Specifically for x86 optimization.
 * Corresponds to: add byte|word|dword|qword ptr [base + index*elem_sz + ofs], val
 */
void tcg_gen_add_cisc_i64(TCGv_i64 base, TCGv_i64 index,  TCGv_i64 val, tcg_target_long elem_sz, tcg_target_long ofs)
{
    if(TCG_TARGET_HAS_add_cisc_i64) {
        tcg_gen_op5(INDEX_op_add_cisc_i64, tcgv_i64_arg(base), tcgv_i64_arg(index), tcgv_i64_arg(val), 
                    tcgv_i64_arg(tcg_constant_i64(elem_sz)), tcgv_i64_arg(tcg_constant_i64(ofs)));
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
 * vCPU...edge_cov_rec_buf[ CRC32(pc|out_edge_id) ] += 1
 */
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id, bool discard_pc) {

    TCGv_i64 hashofs = tcg_temp_ebb_new_i64();
    TCGv_ptr baseptr = tcg_temp_ebb_new_ptr();
    TCGv_i64 en = tcg_temp_ebb_new_i64();

    /* 15 was chosen because most VAs should be smaller than 64-15 bits*/
    tcg_gen_shli_i64(hashofs, pc, 15);
    if(discard_pc)
        tcg_gen_discard_i64(pc);
    tcg_gen_xor_i64(hashofs, hashofs, out_edge_id);
    tcg_gen_fast_hash_i64((TCGv_i32)hashofs, hashofs);

    tcg_gen_ld_ptr(baseptr, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_cov_rec_buf) -
                                        (int) sizeof(CPUNegativeOffsetState)));
    tcg_gen_ld_i32((TCGv_i32)en, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_coverage_enabled) -
                                        (int) sizeof(CPUNegativeOffsetState)));

    tcg_gen_andi_i64(hashofs, hashofs, edge_coverage_record_elems -1);

    tcg_gen_add_cisc_i64((TCGv_i64)baseptr, hashofs, en, edge_coverage_record_elem_size, 0);

    tcg_temp_free_i64(hashofs);
    tcg_temp_free_ptr(baseptr);
    tcg_temp_free_i64(en);
}

void tcg_gen_rec_cmp_i64(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc) {

    TCGv_i64 tcmp = tcg_temp_ebb_new_i64();
    TCGv_i64 tbyte = tcg_temp_ebb_new_i64();
    TCGv_i64 en;
    TCGv_ptr base;
    TCGv_i64 hashofs = tcg_temp_ebb_new_i64();

    tcg_gen_fast_hash_i64((TCGv_i32)hashofs, pc);
    if(discard_pc)
        tcg_gen_discard_i64(pc);

    tcg_gen_xor_i64(tcmp, a0, a1);

    tcg_gen_andi_i64(hashofs, hashofs, comp_coverage_record_elems -1);
    
    for(int i=0; i<8; i++){
        tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
        tcg_gen_setcondi_i64(TCG_COND_EQ, tbyte, tbyte, 0);

        en = tcg_temp_ebb_new_i64();
        tcg_gen_ld_i32((TCGv_i32)en, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.comp_coverage_enabled) -
                                            (int) sizeof(CPUNegativeOffsetState)));
        tcg_gen_and_i64(tbyte, tbyte, en);
        tcg_temp_free_i64(en);

        base = tcg_temp_ebb_new_ptr();
        tcg_gen_ld_ptr(base, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.comp_cov_rec_buf) -
                                            (int) sizeof(CPUNegativeOffsetState)));
        tcg_gen_add_cisc_i64((TCGv_i64)base, hashofs, tbyte, comp_coverage_record_elem_size, 0);
        tcg_temp_free_ptr(base);

        tcg_gen_shri_i64(tcmp, tcmp, 8);
    }

    tcg_temp_free_i64(tcmp);
    tcg_temp_free_i64(tbyte);
    tcg_temp_free_i64(hashofs);
}
