#include "qemu/osdep.h"
#include "tcg/instrument.h"
#include "stdbool.h"
#include "qemu/log.h"

typedef struct  {
	vaddr pc;
	int cpu_index;	/* -1 = matches all vCPU IDs */
	InstrumentCallback cb;
	void *opaque;
} InstrBreakpoint;


struct qht htable;

static bool pc_is_instrumented(const void *p, const void *d) {
	const InstrBreakpoint *a = p;
    const InstrBreakpoint *b = d;

	if( a->pc == b->pc ){
		if( a->cpu_index == -1 || a->cpu_index == b->cpu_index )
			return true;
	}
	return false;
};

static bool cmp(const void *ap, const void *bp)
{
	const InstrBreakpoint *a = ap;
    const InstrBreakpoint *b = bp;

	if( a->pc == b->pc && a->cpu_index == b->cpu_index){
		return true;
	}
	return false;
};

bool check_instrument(vaddr pc, int cpu_index) {

	InstrBreakpoint desc;
	desc.pc = pc;
	desc.cpu_index = cpu_index;

	InstrBreakpoint *b = qht_lookup_custom(&htable, &desc, pc, pc_is_instrumented);
	return b != NULL;
};

bool call_instrument_cb(CPUState *cs, vaddr pc) {
	InstrBreakpoint desc;
	desc.pc = pc;
	desc.cpu_index = cs->cpu_index;

	InstrBreakpoint *b = qht_lookup_custom(&htable, &desc, pc, pc_is_instrumented);

	if(b != NULL) {
		b->cb(cs, pc, b->opaque);
		return true;
	}
	return false;
};

/* Add a instrumentation "breakpoint" to be compiled into intermediate TCG
 * cpu_index = -1 matches all vCPUs
 */
bool add_instrument(vaddr pc, int cpu_index, InstrumentCallback cb, void *opaque) {
	InstrBreakpoint *b = malloc(sizeof(InstrBreakpoint));

	b->pc = pc;
	b->cpu_index = cpu_index;
	b->cb = cb;
	b->opaque = opaque;

	void *existing = NULL;
	qht_insert(&htable, (void *) b, pc, &existing);

	return existing == NULL;
};

bool remove_instrument(vaddr pc, int cpu_index) {
	InstrBreakpoint b;

	b.pc = pc;
	b.cpu_index = cpu_index;

	void *ht_elem = qht_lookup(&htable, &b, pc);
	if( ht_elem != NULL ){
		qht_remove(&htable, ht_elem, pc);
		return true;
	}
	return false;
};

void init_instrument_htable(void) {
	qht_init(&htable, &cmp, 1<<15, QHT_MODE_AUTO_RESIZE);
};


void instrument_cb_overwrite(CPUState *cs, vaddr pc, void *opaque)
{
	qemu_log_mask(LOG_TRACE, "HIT instrument @%llx cpu %d\n", pc, cs->cpu_index);

	RegisterOverwrite *overwrites = (RegisterOverwrite*)opaque;
	for (RegisterOverwrite *o = overwrites; o->tgt_offset != 0; o++) {
		uint8_t *tgt = (uint8_t *)cs + o->tgt_offset;
		uint64_t value = o->value;
		if (o->src_offset != 0)
		{
			uint8_t *src = (uint8_t *)cs + o->src_offset;
			value = 0;
			if (o->sz == 1)
				value = *src;
			if (o->sz == 2)
				value = *(uint16_t *)src;
			if (o->sz == 4)
				value = *(uint32_t *)src;
			if (o->sz == 8)
				value = *(uint64_t *)src;
		}
		if(o->sz == 1)
			*tgt = value & 0xFF;
		if(o->sz == 2)
			*(uint16_t *)tgt = value & 0xFFFF;
		if(o->sz == 4)
			*(uint32_t *)tgt = value & 0xFFFFFFFF;
		if(o->sz == 8)
			*(uint64_t *)tgt = value;
	}
}