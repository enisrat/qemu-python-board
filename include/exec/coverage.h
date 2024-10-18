#ifndef COVERAGE_H
#define COVERAGE_H

/* Max size of the temporary linear buffer for recording hashes of edges */
#define COVERAGE_TEMP_BUF_SZ (1<<24)

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

/* TCG JIT engine will not produce target code for coverage recording if this is false*/
extern bool coverage_record_enabled;

/* Size of the temporary linear buffer */
extern size_t record_temp_buf_size;

/* Informational only, the max size of the temp buf ever */
extern size_t edge_temp_buf_max_size_seen;
extern size_t comp_temp_buf_max_size_seen;

/**
 * Simple linearly filled buffer to record 32-bit values, thus cache efficient recording.
 * Buffer is of size @record_temp_buf_size.
 */
typedef struct {
    uint32_t *start;
    uint32_t *next;
} RecordValuesTempBuf;

/**
 * Each vCPU gets one of these for direct access from TCG target code.
 */
typedef struct {
    /* Should be 0 or sizeof(uint32_t). Atomic Writes, Read by TCG target code */
    uint32_t edge_coverage_enabled;
    uint32_t comp_coverage_enabled;

    /** The actual target hit maps used by AFL etc.
     * Will be filled during runtime from the temporary buffers below
    */
    void *edge_cov_hitmap;
    void *comp_cov_hitmap;

    /* Temporary buffer of hashed edge elements for coverage recording */
    RecordValuesTempBuf edge_temp_buf;
    RecordValuesTempBuf comp_temp_buf;

} CoverageRecordBuf;

/* Initializes the coverage record buffers based on the sizes (static globals as *extern*) above. */
int init_coverage_recording(void *opaque, QemuOpts *opts, Error **errp);


/**
 * Transfer the temporary buffers to the hitmaps.
 * This should be called regularly to avoid temp buffer overflow.
 */
void transfer_covrec_temp_bufs_to_hitmaps(CoverageRecordBuf* buf);

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
