/**
 * Edge Coverage Recording
 * 
 * (Needs aligned_alloc)
 */

#include <unistd.h>
#include "qemu/osdep.h"
#include "hw/core/cpu.h"
#include "exec/coverage.h"

/**
* Globally used parameters. Initialized at beginning, never changed afterward.
*/
size_t edge_coverage_record_elem_size = 1;
/*edge_coverage_record_size = #elems * elem_sz */
size_t edge_coverage_record_elems = 4096;

size_t comp_coverage_record_elem_size = 1;
/*comp_coverage_record_size = #elems * elem_sz */
size_t comp_coverage_record_elems = 4096;

bool coverage_record_enabled = false;

void init_coverage_recording(void) {
    
    /* buffer sizes need to be powers of 2*/
    assert((edge_coverage_record_elems & (edge_coverage_record_elems - 1)) == 0 );
    assert((comp_coverage_record_elems & (comp_coverage_record_elems - 1)) == 0 );
    /* elem sizes are byte...qword */
    assert(edge_coverage_record_elem_size == 1 || 
            edge_coverage_record_elem_size == 2 || 
            edge_coverage_record_elem_size == 4|| 
            edge_coverage_record_elem_size == 8);
    assert(comp_coverage_record_elem_size == 1 || 
            comp_coverage_record_elem_size == 2 || 
            comp_coverage_record_elem_size == 4|| 
            comp_coverage_record_elem_size == 8);

    CPUState *cpu;
    /* all CPUs have same coverage params*/
    CPU_FOREACH(cpu) {
        cpu->neg.coverage_rec.edge_cov_rec_buf = aligned_alloc(0x1000, edge_coverage_record_elems * edge_coverage_record_elem_size);
        cpu->neg.coverage_rec.comp_cov_rec_buf = aligned_alloc(0x1000, comp_coverage_record_elems * comp_coverage_record_elem_size);
        cpu->neg.coverage_rec.comp_coverage_enabled = 1;
        cpu->neg.coverage_rec.edge_coverage_enabled = 1;
    }

    coverage_record_enabled = true;
}

void enable_edge_coverage_single_cpu(CoverageRecordBuf* buf) {
    qatomic_set(&buf->edge_coverage_enabled, 1);
}

void disable_edge_coverage_single_cpu(CoverageRecordBuf* buf) {
    qatomic_set(&buf->edge_coverage_enabled, 0);
}
void enable_comp_coverage_single_cpu(CoverageRecordBuf* buf) {
    qatomic_set(&buf->comp_coverage_enabled, 1);
}

void disable_comp_coverage_single_cpu(CoverageRecordBuf* buf) {
    qatomic_set(&buf->comp_coverage_enabled, 0);
}
void enable_edge_coverage_all_cpus(void){
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        enable_edge_coverage_single_cpu(&cpu->neg.coverage_rec);
    }
}

void disable_edge_coverage_all_cpus(void) {
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        disable_edge_coverage_single_cpu(&cpu->neg.coverage_rec);
    }
}
void enable_comp_coverage_all_cpus(void) {
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        enable_comp_coverage_single_cpu(&cpu->neg.coverage_rec);
    }
}

void disable_comp_coverage_all_cpus(void) {
    CPUState *cpu;
    CPU_FOREACH(cpu) {
        disable_comp_coverage_single_cpu(&cpu->neg.coverage_rec);
    }
}