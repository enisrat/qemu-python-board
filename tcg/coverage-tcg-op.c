
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
        tcg_gen_op5(INDEX_op_add_cisc_i64, tcgv_i64_arg(base), tcgv_i64_arg(index), tcgv_i64_arg(val), elem_sz, ofs);
    } else {
        tcg_debug_assert(elem_sz == 1 || elem_sz == 2 || elem_sz == 4 || elem_sz == 8);
        TCGv_i64 t0 = tcg_temp_new_i64();
        TCGv_i64 t1 = tcg_temp_new_i64();

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
void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id, bool discard_pc) {
    if(coverage_record_enabled) {

        TCGv_i64 hashofs = tcg_temp_new_i64();
        tcg_gen_fast_hash_i64((TCGv_i32)hashofs, out_edge_id, pc); 
        if(discard_pc)
            tcg_gen_discard_i64(pc);

        TCGv_ptr baseptr = tcg_temp_new_ptr();
        tcg_gen_ld_ptr(baseptr, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_rec.rec_buf_hitmap) -
                                            (int) sizeof(CPUNegativeOffsetState)));
        TCGv_i64 mask = tcg_temp_new_i64();
        tcg_gen_ld_i32((TCGv_i32)mask, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.edge_rec.mask) -
                                            (int) sizeof(CPUNegativeOffsetState)));

        tcg_gen_and_i64(hashofs, hashofs, mask);
        if(edge_coverage_record_elem_size == 2) {
            tcg_gen_shl_i32(hashofs, hashofs, 1);
        } else if(edge_coverage_record_elem_size == 4) {
            tcg_gen_shl_i32(hashofs, hashofs, 2);
        }
        tcg_gen_add_i64(baseptr, baseptr, hashofs);

        assert(edge_coverage_record_elem_size  == 1); //@TODO fix for other sizes
        //inc [baseptr]
        TCGv_i64 ctr = tcg_temp_new_i64();
        tcg_gen_ld8u_i64(ctr, baseptr, 0);
        tcg_gen_add_i64(ctr, ctr, tcg_constant_i64(1));
        tcg_gen_st8_i64(ctr, baseptr, 0);

        /*
        tcg_temp_free_i64(hashofs);
        tcg_temp_free_ptr(baseptr);
        tcg_temp_free_i64(mask);*/
    }
}

void tcg_gen_rec_cmp_i64(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1, bool discard_pc) {
    if(coverage_record_enabled) {

        TCGv_i64 tcmp = tcg_temp_new_i64();
        TCGv_i64 tbyte = tcg_temp_new_i64();
        TCGv_i64 mask = tcg_temp_new_i64();
        TCGv_ptr base = tcg_temp_new_ptr();
        TCGv_i64 hashofs = tcg_temp_new_i64();

        tcg_gen_mov_i64(hashofs, tcg_constant_i64(0));
        tcg_gen_fast_hash_i64((TCGv_i32)hashofs, hashofs, pc);
        if(discard_pc)
            tcg_gen_discard_i64(pc);

        tcg_gen_xor_i64(tcmp, a0, a1);

        tcg_gen_ld_ptr(base, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.comp_rec.rec_buf_hitmap) -
                                            (int) sizeof(CPUNegativeOffsetState)));
        tcg_gen_ld_i32((TCGv_i32)mask, tcg_env, ((int) offsetof(CPUNegativeOffsetState, coverage_rec.comp_rec.mask) -
                                            (int) sizeof(CPUNegativeOffsetState)));

        tcg_gen_and_i64(hashofs, hashofs, mask);
        if(comp_coverage_record_elem_size == 2) {
            tcg_gen_shl_i32(hashofs, hashofs, 1);
        } else if(comp_coverage_record_elem_size == 4) {
            tcg_gen_shl_i32(hashofs, hashofs, 2);
        }

        tcg_gen_add_i64(base, base, hashofs);
        TCGv_i64 ctr = tcg_temp_new_i64();

        for(int i=0; i<8; i++){
            tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
            tcg_gen_setcondi_i64(TCG_COND_EQ, tbyte, tbyte, 0);

            assert(comp_coverage_record_elem_size  == 1); //@TODO fix for other sizes
            
            //add [base], tbyte
            tcg_gen_ld8u_i64(ctr, base, 0);
            tcg_gen_add_i64(ctr, ctr, tbyte);
            tcg_gen_st8_i64(ctr, base, 0);

            tcg_gen_shri_i64(tcmp, tcmp, 8);
        }

        /*tcg_temp_free_i64(tcmp);
        tcg_temp_free_i64(tbyte);
        tcg_temp_free_i64(hashofs);*/
    }
}
