/**
 * Coverage Recording for edges (and compare values)
 * 
 * (Needs aligned_alloc)
 */

#include <unistd.h>
#include "qemu/osdep.h"
#include "hw/core/cpu.h"
#include "exec/coverage.h"
#include "qapi/qmp/qdict.h"
#include "qapi/error.h"
#include "monitor/hmp.h"
#include "monitor/monitor.h"
#include "monitor/hmp-target.h"
#include "qemu/option.h"
#include "qemu/config-file.h"

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

int init_coverage_recording(void *opaque, QemuOpts *opts, Error **errp) {

    edge_coverage_record_elem_size = qemu_opt_get_number(opts, "edge_elem_sz", 1);
    edge_coverage_record_elems = qemu_opt_get_number(opts, "edge_elems", 4096);

    comp_coverage_record_elem_size = qemu_opt_get_number(opts, "comp_elem_sz", 1);
    comp_coverage_record_elems = qemu_opt_get_number(opts, "comp_elems", 4096);
    
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
        reset_edge_coverage_single_cpu(&cpu->neg.coverage_rec);
        cpu->neg.coverage_rec.comp_cov_rec_buf = aligned_alloc(0x1000, comp_coverage_record_elems * comp_coverage_record_elem_size);
        reset_comp_coverage_single_cpu(&cpu->neg.coverage_rec);
        cpu->neg.coverage_rec.comp_coverage_enabled = qemu_opt_get_bool(opts, "comp_enable", false);
        cpu->neg.coverage_rec.edge_coverage_enabled = qemu_opt_get_bool(opts, "edge_enable", false);
    }

    coverage_record_enabled = true;

    return 0;
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

void reset_edge_coverage_single_cpu(CoverageRecordBuf* buf) {
    memset(buf->edge_cov_rec_buf, 0, edge_coverage_record_elems * comp_coverage_record_elem_size);
}
void reset_comp_coverage_single_cpu(CoverageRecordBuf* buf) {
    memset(buf->comp_cov_rec_buf, 0, comp_coverage_record_elems * comp_coverage_record_elem_size);
}


static void monitor_dump(Monitor *mon, void *where, int count, int wsize) 
{
    uint8_t *buf = (uint8_t*)where;

    int l, line_size, i, max_digits, len;
    uint64_t v;
    
    len = wsize * count;
    line_size = 16;

    max_digits = (wsize * 8) / 4;

    int pos=0;
    while (len > 0) {
        
        l = len;
        if (l > line_size)
            l = line_size;
        i = 0;
        while (i < l) {
            v=0;
            for(int j=0; j<wsize; j++) {
                v |= *((uint8_t*)buf+pos) << (j*8);
                pos++;
            }
            monitor_printf(mon, "0x%0*" PRIx64, max_digits, v);
            if(i<l) monitor_printf(mon, " ");
            i += wsize;
        }
        monitor_printf(mon, "\n");
        len -= l;
    }
}

void hmp_covrec_dump_edge_buf(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    monitor_dump(mon, cpu->neg.coverage_rec.edge_cov_rec_buf, edge_coverage_record_elems, edge_coverage_record_elem_size);
}

void hmp_covrec_dump_comp_buf(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    monitor_dump(mon, cpu->neg.coverage_rec.comp_cov_rec_buf, comp_coverage_record_elems, comp_coverage_record_elem_size);
}

void hmp_covrec_set_edge_enabled(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");
    int enabled = qdict_get_int(qdict, "enabled");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    if(enabled)
        enable_edge_coverage_single_cpu(&cpu->neg.coverage_rec);
    else
        disable_edge_coverage_single_cpu(&cpu->neg.coverage_rec);
}

void hmp_covrec_is_enabled_edge(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    uint8_t is_enabled = qatomic_read(&cpu->neg.coverage_rec.edge_coverage_enabled);
    monitor_printf(mon, "%d\n", is_enabled);
}

void hmp_covrec_set_comp_enabled(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");
    int enabled = qdict_get_int(qdict, "enabled");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    if(enabled)
        enable_comp_coverage_single_cpu(&cpu->neg.coverage_rec);
    else
        disable_comp_coverage_single_cpu(&cpu->neg.coverage_rec);
}
void hmp_covrec_is_enabled_comp(Monitor *mon, const QDict *qdict)
{
    Error *err = NULL;
    int cpui = qdict_get_int(qdict, "cpu");

    CPUState* cpu = qemu_get_cpu(cpui);

    if (!cpu) {
        error_setg(&err, "CPU %d not found", cpui);
        hmp_handle_error(mon, err);
        return;
    }

    uint8_t is_enabled = qatomic_read(&cpu->neg.coverage_rec.comp_coverage_enabled);
    monitor_printf(mon, "%d\n", is_enabled);
}


//[edge_elem_sz=n][,edge_elems=n][,comp_elem_sz=n][,comp_elems=n][,edge_enable][,comp_enable]
static QemuOptsList qemu_covrec_opts = {
    .name = "covrec",
    .head = QTAILQ_HEAD_INITIALIZER(qemu_covrec_opts.head),
    .implied_opt_name = "covrec",
    .desc = {
        {
            .name = "edge_elem_sz",
            .type = QEMU_OPT_NUMBER,
        },
        {
            .name = "edge_elems",
            .type = QEMU_OPT_NUMBER,
        },
        {
            .name = "comp_elem_sz",
            .type = QEMU_OPT_NUMBER,
        },
        {
            .name = "comp_elems",
            .type = QEMU_OPT_NUMBER,
        },
        {
            .name = "edge_enable",
            .type = QEMU_OPT_BOOL,
        },
        {
            .name = "comp_enable",
            .type = QEMU_OPT_BOOL,
        },
        { /* end of list */ }
    },
};
static void covrec_register_config(void)
{
    qemu_add_opts(&qemu_covrec_opts);
}
opts_init(covrec_register_config);
