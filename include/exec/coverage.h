#define EDGE_COVERAGE_RECORD_BITS 12
#define EDGE_COVERAGE_RECORD_NUM_ELEMS (1<< EDGE_COVERAGE_RECORD_BITS)

#define EDGE_COVERAGE_RECORD_ELEM_SIZE 1

#if EDGE_COVERAGE_RECORD_ELEM_SIZE == 1
	typedef uint8_t edge_cov_rec_elem_t;
#elif EDGE_COVERAGE_RECORD_ELEM_SIZE == 2
	typedef uint16_t edge_cov_rec_elem_t;
#endif

#define EDGE_COVERAGE_RECORD_SIZE (EDGE_COVERAGE_RECORD_NUM_ELEMS * EDGE_COVERAGE_RECORD_ELEM_SIZE)

typedef struct {
    edge_cov_rec_elem_t edge_cov_rec_buf[EDGE_COVERAGE_RECORD_SIZE];
} CoverageRecordBuf;