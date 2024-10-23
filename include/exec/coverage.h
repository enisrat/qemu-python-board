#ifndef COVERAGE_H
#define COVERAGE_H

/**
 * These global parameters should NOT be changed after initialization (init_coverage_recording).
 * The reason is that the values are used in TCG target code and should not be changed at runtime.
 * 
 * The elem_size refers to the size of each counter and can be 1,2 or 4 (bytes)
 */
extern size_t edge_coverage_record_elem_size;
/*edge_coverage_record_size = #elems * elem_sz */
extern size_t edge_coverage_record_elems;

extern size_t comp_coverage_record_elem_size;
/*comp_coverage_record_size = #elems * elem_sz */
extern size_t comp_coverage_record_elems;

/* TCG JIT engine will not produce target code for coverage recording if this is false*/
extern bool coverage_record_enabled;

/* Initializes the coverage record buffers based on the sizes (static globals as *extern*) above. */
int init_coverage_recording(void *opaque, QemuOpts *opts, Error **errp);

typedef struct {
    /** Mask to be applied to an "edge hash". 
      * A mask=0 means coverage recording is DISABLED. Enable/Disable should be done using below functions only. 
    */
    uint32_t mask;
    /**
     * @brief Whenever the mask is set to 0, recording will always update the first value rec_buf_hitmap[0]
     * This value is thus cached when disabling and restored when enabling.
     */
    uint32_t cached_first_value;
    /**
     * @brief Pointer to actual buffer containing the "hitmap" of edges as in AFL.
     * 
     */
    void *rec_buf_hitmap;
} CoverageRecordBuf;

/**
 * Each vCPU gets one of these for direct access from TCG target code.
 */
typedef struct {

    CoverageRecordBuf edge_rec;
    CoverageRecordBuf comp_rec;

} CoverageRecordBufs;

/**
 * Enabling and disabling coverage recording can be used from any thread and is done via atomic write.
 * en/disabling for all vCPUs at once should be done while machine is stopped.
 */
void enable_edge_coverage_single_cpu(CoverageRecordBufs* buf);
void disable_edge_coverage_single_cpu(CoverageRecordBufs* buf); 
void enable_comp_coverage_single_cpu(CoverageRecordBufs* buf);
void disable_comp_coverage_single_cpu(CoverageRecordBufs* buf);
void enable_edge_coverage_all_cpus(void);
void disable_edge_coverage_all_cpus(void);
void enable_comp_coverage_all_cpus(void);
void disable_comp_coverage_all_cpus(void);

void reset_edge_coverage_single_cpu(CoverageRecordBufs* buf);
void reset_comp_coverage_single_cpu(CoverageRecordBufs* buf);

#endif // COVERAGE_H
