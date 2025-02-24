#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/units.h"
#include "hw/boards.h"
#include "exec/memory.h"
#include "target/arm/cpu-qom.h"
#include "target/arm/gtimer.h"
#include "hw/arm/bsa.h"
#include "exec/address-spaces.h"
#include "hw/core/cpu.h"
#include "qapi/qmp/qlist.h"
#include "target/arm/cpu.h"
#include "tcg/instrument.h"
#include "crypto/hash.h"
#include "qemu/log.h"


static void retN(CPUState *cs, vaddr pc, void *opaque)
{
    qemu_log_mask(LOG_TRACE, "HIT instrument @%llx cpu %d %llx\n", pc, cs->cpu_index, opaque);
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = (uint64_t)opaque;
    cpu->env.pc = cpu->env.xregs[30];
}
static void patchXBL_SEC_Upper_Bound_func(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[17] = 0x148fffff;
}
static void fix_PMD_for_Secmon_func(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[1] = 0x1;
}

static int hashmode2 = 0;
static char *hashbuf2 = NULL; //just big enough for the largest hash
static char *hash_next_update2;

void hash_init(CPUState *cs, vaddr pc, void *opaque)
{
    if(!hashbuf2){
        hashbuf2 = g_malloc(1<<22);
    }
    ARMCPU *cpu = ARM_CPU(cs);
    hashmode2 = cpu->env.xregs[1];
    hash_next_update2 = hashbuf2;
    if (hashmode2 != 2 && hashmode2 != 3) {
        qemu_log_mask(LOG_TRACE, "hash_init unknown mode %d\n", hashmode2);
        return;
    }
    qemu_log_mask(LOG_TRACE, "hash_init %d\n", hashmode2);
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void hash_update(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t src = ldq_le_phys(&address_space_memory, cpu->env.xregs[1]);
    uint64_t len = ldq_le_phys(&address_space_memory, cpu->env.xregs[1]+8);
    qemu_log_mask(LOG_TRACE, "hash_update len %d\n", len);
    cpu_memory_rw_debug(cs, src, hash_next_update2, len, false);
    hash_next_update2 += len;
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void hash_finish(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t dst = ldq_le_phys(&address_space_memory, cpu->env.xregs[1]);
    dst = ldq_le_phys(&address_space_memory, dst);   //no mistake

    char *digest = NULL;
    QCryptoHashAlgorithm qalg;
    size_t sz = 0;
    if (hashmode2 == 2)
        qalg = QCRYPTO_HASH_ALG_SHA256;
    else if (hashmode2 == 3) 
        qalg =  QCRYPTO_HASH_ALG_SHA384;
    else {
        qemu_log_mask(LOG_TRACE, "hash_finish unknown mode %d\n", hashmode2);
        return;
    }
    qcrypto_hash_bytes(qalg, hashbuf2, hash_next_update2 - hashbuf2, &digest, &sz, &error_fatal);

    cpu_memory_rw_debug(cs, dst, digest, sz, true);
    qemu_log_mask(LOG_TRACE, "hash_finish len %d\n", sz);
    g_free(digest);
    hash_next_update2 = hashbuf2;
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

void brom_instrument()
{
    add_instrument(0x302A08, -1, retN, 0); // pbl_hw_init
    add_instrument(0x30F9E4, -1, retN, 1); // some clk control??
    add_instrument(0x303660, -1, patchXBL_SEC_Upper_Bound_func, 0);
    add_instrument(0x302344, -1, fix_PMD_for_Secmon_func, 0);
    
    add_instrument(0x31DB9C, -1, hash_init, NULL);
    add_instrument(0x31DE0C, -1, hash_update, NULL);
    add_instrument(0x31E048, -1, hash_finish, NULL);

}