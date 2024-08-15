#ifndef COVERAGE_H
#define COVERAGE_H

extern size_t edge_coverage_record_elem_size;
/*edge_coverage_record_size = #elems * elem_sz */
extern size_t edge_coverage_record_elems;

extern size_t comp_coverage_record_elem_size;
/*comp_coverage_record_size = #elems * elem_sz */
extern size_t comp_coverage_record_elems;

extern bool coverage_record_enabled;

/* Initializes the coverage record buffers based on the sizes (static globals as *extern*) above. */
void init_coverage_recording(void);

/**
 * Each vCPU gets one of these for direct access from TCG target code.
 */
typedef struct {
    /* Should be 0 or 1 */
    uint32_t edge_coverage_enabled;
    uint32_t comp_coverage_enabled;

    /* Should be page-aligned for max cache efficiency */
    void* edge_cov_rec_buf;
    void* comp_cov_rec_buf;

} CoverageRecordBuf;

#endif // COVERAGE_H
