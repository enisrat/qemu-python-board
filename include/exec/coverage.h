#ifndef COVERAGE_H
#define COVERAGE_H

/**
 * These global parameters should NOT be changed after initialization (init_coverage_recording).
 * The reason is that the values are used in TCG target code and should not be changed at runtime.
 * 
 * The elem_size refers to the size of each counter and can be 1,2,4 or 8 (bytes)
 */
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

    /* Buffer of counter elements for coverage recording. Should be page-aligned for max cache efficiency */
    void* edge_cov_rec_buf;
    void* comp_cov_rec_buf;

} CoverageRecordBuf;

/**
 * Enabling and disabling coverage recording can be used from any thread and is done via atomic write.
 * en/disabling for all vCPUs at once should be done while machine is stopped.
 */
void enable_edge_coverage_single_cpu(CoverageRecordBuf* buf);
void disable_edge_coverage_single_cpu(CoverageRecordBuf* buf); 
void enable_comp_coverage_single_cpu(CoverageRecordBuf* buf);
void disable_comp_coverage_single_cpu(CoverageRecordBuf* buf);
void enable_edge_coverage_all_cpus(void);
void disable_edge_coverage_all_cpus(void);
void enable_comp_coverage_all_cpus(void);
void disable_comp_coverage_all_cpus(void);

#endif // COVERAGE_H
