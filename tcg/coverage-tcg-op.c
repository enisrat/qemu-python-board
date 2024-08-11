
#include "qemu/osdep.h"
#include "tcg/tcg.h"
#include "tcg/tcg-temp-internal.h"
#include "tcg/tcg-op-common.h"
#include "tcg-internal.h"


static inline cov_rec_buf_ofs()
{
    return ();
}

static inline cov_rec_buf_ofs()
{
    return (offsetof(CPUNegativeOffsetState, edge_cov_rec_buf) -
            sizeof(CPUNegativeOffsetState));
}

void tcg_gen_rec_edge_i64(TCGv_i64 pc, TCGv_i64 out_edge_id) {
    TCGv_i64 t0, t1;

    t0 = tcg_temp_ebb_new_i64();
    t1 = tcg_temp_ebb_new_i64();

    /* 15 was chosen because most VAs should be smaller than 64-15 bits*/
    tcg_gen_shifti_i64(t0, pc, 15, false, false);
    tcg_gen_xor_i64(t0, t0, out_edge_id);
    tcg_gen_fast_hash_i64(t0, t0);

    tcg_gen_add_i64(t1, tcg_env, offsetof(CPUNegativeOffsetState, edge_cov_rec_buf) -
            sizeof(CPUNegativeOffsetState));
    tcg_gen_and_i64(t0, EDGE_COVERAGE_RECORD_NUM_ELEMS -1);

    #if EDGE_COVERAGE_RECORD_ELEM_SIZE > 1
        tcg_gen_shifti_i64(t0, t0, EDGE_COVERAGE_RECORD_ELEM_SIZE-1, false, false);
    #endif

    tcg_gen_add_i64(t0, t1, 0);
    /* OPTIMIZE?: INC byte|word ptr[t0]*/
    tcg_gen_ld8_i64(t1, t0, 0);
    tcg_gen_add_i64(t1, t1, 1);
    tcg_gen_st8_i64(t1, t0, 0);
}

void tcg_gen_rec_cmp(TCGv_i64 pc, TCGv_i64 a0, TCGv_i64 a1) {
    TCGv_i64 tcmp, tbyte, tbytecond;

    int ofs = offsetof(CPUNegativeOffsetState, comp_cov_rec_buf) -
            sizeof(CPUNegativeOffsetState);

    tcmp = tcg_temp_ebb_new_i64();
    tbyte = tcg_temp_ebb_new_i64();
    tbytecond = tcg_temp_ebb_new_i64();
    tindex = tcg_temp_ebb_new_i64();

    tcg_gen_fast_hash_i64(tindex, pc);

    /* 15 was chosen because most VAs should be smaller than 64-15 bits*/
    tcg_gen_xor_i64(tcmp, a0, a1);
    tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
    tcg_gen_setcondi_i64(TCG_COND_EQ, tbytecond, tbyte, 0);
    tcg_gen_discard_i64(tbyte);

    tcg_gen_and_i64(tindex, tindex, EDGE_COVERAGE_RECORD_NUM_ELEMS -1);
    tcg_gen_add_i64(tindex, tindex, tcg_env);

    /* OPTIMIZE */
    tcg_gen_ld8_i64(tbyte, tindex, ofs);
    tcg_gen_add_i64(tbyte, tbyte, tbytecond);
    tcg_gen_st8_i64(tbyte, tindex, ofs);
    tcg_gen_discard_i64(tbyte);

    for(int i=0; i<7; i++){
        tcg_gen_shifti_i64(tcmp, tcmp, 8, true, false);
        tcg_gen_extract_i64(tbyte, tcmp, 0, 8);
        tcg_gen_setcondi_i64(TCG_COND_EQ, tbytecond, tbyte, 0);
        tcg_gen_ld8_i64(tbyte, tindex, ofs);
        tcg_gen_add_i64(tbyte, tbyte, tbytecond);
        tcg_gen_st8_i64(tbyte, tindex, ofs);
    }
}
