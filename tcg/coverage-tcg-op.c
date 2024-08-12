
#include "qemu/osdep.h"
#include "tcg/tcg.h"
#include "tcg/tcg-temp-internal.h"
#include "tcg/tcg-op-common.h"
#include "tcg-internal.h"


static inline int edge_cov_rec_buf_ofs(void)
{
    return ((int) (offsetof(CPUNegativeOffsetState, cov_rec_buf.edge_cov_rec_buf)) -
            (int) sizeof(CPUNegativeOffsetState) );
}

static inline int comp_cov_rec_buf_ofs(void)
{
    return ((int) (offsetof(CPUNegativeOffsetState, cov_rec_buf.comp_cov_rec_buf)) -
            (int) sizeof(CPUNegativeOffsetState) );
}

void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id) {
    TCGv_i64 t0, t1;

    t0 = tcg_temp_ebb_new_i64();
    t1 = tcg_temp_ebb_new_i64();

    /* 15 was chosen because most VAs should be smaller than 64-15 bits*/
    tcg_gen_shli_i64(t0, pc, 15);
    tcg_gen_discard_i64(pc);
    tcg_gen_xor_i64(t0, t0, out_edge_id);
    tcg_gen_fast_hash_i64((TCGv_i32)t0, t0);

    tcg_gen_addi_i64(t1, (TCGv_i64)tcg_env, edge_cov_rec_buf_ofs());
    tcg_gen_andi_i64(t0, t0, EDGE_COVERAGE_RECORD_NUM_ELEMS -1);

    #if EDGE_COVERAGE_RECORD_ELEM_SIZE > 1
        tcg_gen_shli_i64(t0, t0, EDGE_COVERAGE_RECORD_ELEM_SIZE-1);
    #endif

    tcg_gen_add_i64(t0, t0, t1);
    /* OPTIMIZE?: INC byte|word ptr[t0]*/
    tcg_gen_ld8u_i64(t1, (TCGv_ptr)t0, 0);
    tcg_gen_addi_i64(t1, t1, 1);
    tcg_gen_st8_i64(t1, (TCGv_ptr)t0, 0);

    tcg_temp_free_i64(t0);
    tcg_temp_free_i64(t1);
}

void tcg_gen_rec_cmp_i64(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1) {
    TCGv_i64 tcmp, tbyte, tbytecond, tindex;

    int ofs = comp_cov_rec_buf_ofs();

    tcmp = tcg_temp_ebb_new_i64();
    tbyte = tcg_temp_ebb_new_i64();
    tbytecond = tcg_temp_ebb_new_i64();
    tindex = tcg_temp_ebb_new_i64();

    tcg_gen_fast_hash_i64((TCGv_i32)tindex, pc);
    tcg_gen_discard_i64(pc);

    /* 15 was chosen because most VAs should be smaller than 64-15 bits*/
    tcg_gen_xor_i64(tcmp, a0, a1);
    tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
    tcg_gen_setcondi_i64(TCG_COND_EQ, tbytecond, tbyte, 0);
    tcg_gen_discard_i64(tbyte);

    tcg_gen_andi_i64(tindex, tindex, EDGE_COVERAGE_RECORD_NUM_ELEMS -1);
    tcg_gen_add_i64(tindex, tindex, (TCGv_i64)tcg_env);

    /* OPTIMIZE */
    tcg_gen_ld8u_i64(tbyte, (TCGv_ptr)tindex, ofs);
    tcg_gen_add_i64(tbyte, tbyte, tbytecond);
    tcg_gen_st8_i64(tbyte, (TCGv_ptr)tindex, ofs);
    tcg_gen_discard_i64(tbyte);

    for(int i=0; i<7; i++){
        tcg_gen_shri_i64(tcmp, tcmp, 8);
        tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
        tcg_gen_setcondi_i64(TCG_COND_EQ, tbytecond, tbyte, 0);
        tcg_gen_ld8u_i64(tbyte, (TCGv_ptr)tindex, ofs);
        tcg_gen_add_i64(tbyte, tbyte, tbytecond);
        tcg_gen_st8_i64(tbyte, (TCGv_ptr)tindex, ofs);
    }

    tcg_temp_free_i64(tcmp);
    tcg_temp_free_i64(tbyte);
    tcg_temp_free_i64(tbytecond);
    tcg_temp_free_i64(tindex);
}
