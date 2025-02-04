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

    /**
    * ICBCFG memory region types
    */
    typedef struct
    {
    uint64_t base_addr;
    uint64_t size;
    uint64_t   interleaved;
    } region_type;
    /* DDR channel definitions */
    #define MAX_REGIONS 6
    typedef struct
    {
    region_type regions[MAX_REGIONS];
    } regions_type;
    /* DDR slave region configuration */
    #define MAX_CHANNELS 2
    typedef struct
    {
    regions_type channels[MAX_CHANNELS];
    } icb_mem_map_type;

static void ICB_Get_Memmap(CPUState *cs, vaddr pc, void *opaque)
{
    icb_mem_map_type mem_map = {
        .channels = {
            [0] = {
                .regions = {
                    [0] = { .base_addr = 0x80000000, .size = 0x80000000, .interleaved = 0 },
                    [1] = { .base_addr = 0x100000000, .size = 0x80000000, .interleaved = 0 },
                }
            },
            [1] = {
                .regions = {
                    [0] = { .base_addr = 0x180000000, .size = 0x80000000, .interleaved = 0 },
                    [1] = { .base_addr = 0x200000000, .size = 0x80000000, .interleaved = 0 }
                }
            }
        }
    };
    ARMCPU *cpu = ARM_CPU(cs);
    cpu_memory_rw_debug(cs, cpu->env.xregs[1], &mem_map, sizeof(mem_map), true);
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void get_possible_DRAM_range(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);

    uint32_t start = 0x80000000;
    uint64_t end = 0x300000000;
    cpu_memory_rw_debug(cs, cpu->env.xregs[0], start, sizeof(start), true);
    cpu_memory_rw_debug(cs, cpu->env.xregs[1], end, sizeof(end), true);
    
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void tz_get_loglevel(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void disable_xpu_ac(CPUState *cs, vaddr pc, void *opaque)
{
    uint32_t flag = 1;
    cpu_memory_rw_debug(cs, 0x887AED30C, flag, sizeof(flag), true);
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.pc = cpu->env.xregs[30];
}

static int hashmode = 0;
static char *hashbuf = NULL; //just big enough for the largest hash
static char *hash_next_update;

static void tzbsp_hash_init(CPUState *cs, vaddr pc, void *opaque)
{
    if(!hashbuf){
        hashbuf = g_malloc(1<<22);
    }
    ARMCPU *cpu = ARM_CPU(cs);
    hashmode = cpu->env.xregs[0];
    hash_next_update = hashbuf;
    if (hashmode != 3 && hashmode != 4) {
        qemu_log_mask(LOG_TRACE, "tzbsp_hash_init unknown mode %d\n", hashmode);
        return;
    }
    qemu_log_mask(LOG_TRACE, "tzbsp_hash_init %d\n", hashmode);
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void tzbsp_hash_update(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t src = ldq_le_phys(&address_space_memory, cpu->env.xregs[1]);
    uint64_t len = cpu->env.xregs[2];
    qemu_log_mask(LOG_TRACE, "tzbsp_hash_update len %d\n", len);
    cpu_memory_rw_debug(cs, src, hash_next_update, len, false);
    hash_next_update += len;
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30];
}

static void tzbsp_hash_final(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t ptr1 = cpu->env.xregs[1];
    char *digest = NULL;
    QCryptoHashAlgorithm qalg;
    size_t sz = 0;
    if (hashmode == 3)
        qalg = QCRYPTO_HASH_ALG_SHA256;
    else if (hashmode == 4) 
        qalg =  QCRYPTO_HASH_ALG_SHA384;
    else {
        qemu_log_mask(LOG_TRACE, "tzbsp_hash_final unknown mode %d\n", hashmode);
        return;
    }
    qcrypto_hash_bytes(qalg, hashbuf, hash_next_update - hashbuf, &digest, &sz, &error_fatal);

    cpu_memory_rw_debug(cs, ptr1, digest, sz, true);
    qemu_log_mask(LOG_TRACE, "tzbsp_hash_final len %d\n", sz);
    g_free(digest);
    hash_next_update = hashbuf;
    cpu->env.xregs[0] = 0;
    cpu->env.pc = cpu->env.xregs[30]; 
}

static void print_smc(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    qemu_log_mask(LOG_TRACE, "EL1 SMC: %d\n", cpu->env.xregs[0]);
}

// externally defined
void hook_qsee_start(CPUState *cs, vaddr pc, void *opaque);

void tz_instrument()
{
    add_instrument(0x887A395C8, -1, ICB_Get_Memmap, NULL); // ICB_Get_Memmap
    add_instrument(0x8879DDD54, -1, get_possible_DRAM_range, NULL); // get_possible_DRAM_range

    /*for ea in [ 0x887A249A8, #rpmh_register_isr
            0x887A200A8, #PDC
            0x887A290EC, #VPP
            0x887A33E10,
            0x887A44264, #VMIDMT stuff (related to XPU, SMMU)
            0x887A55BDC, #SMMU debug config
            0x887A09780, #Qcom IPA (integrated HW IP switch)
            0x887A3CD5C, #AC XPU?
            0x8879CB46C,
            0x887A13330,*/
    add_instrument(0x887A249A8, -1, retN, 0); // rpmh_register_isr
    add_instrument(0x887A200A8, -1, retN, 0); // PDC
    add_instrument(0x887A290EC, -1, retN, 0); // VPP
    add_instrument(0x887A33E10, -1, retN, 0);
    add_instrument(0x887A44264, -1, retN, 0); // VMIDMT stuff (related to XPU, SMMU)
    add_instrument(0x887A55BDC, -1, retN, 0); // SMMU debug config
    add_instrument(0x887A09780, -1, retN, 0); // Qcom IPA (integrated HW IP switch)
    add_instrument(0x887A3CD5C, -1, retN, 0); // AC XPU?
    add_instrument(0x8879CB46C, -1, retN, 0);
    add_instrument(0x887A13330, -1, retN, 0);

    add_instrument(0x8879E3C54, -1, retN, 1); // XPU AC (Access Control)
    add_instrument(0x8879E3D70, -1, retN, 1);

    add_instrument(0x887A3CD5C, -1, disable_xpu_ac, NULL); // Set "disable_xpu_ac" to 1, just to be sure!

    add_instrument(0x887A884CC, -1, tzbsp_hash_init, NULL); // tzbsp_hash_init
    add_instrument(0x887A88554, -1, tzbsp_hash_update, NULL); // tzbsp_hash_update
    add_instrument(0x887A88624, -1, tzbsp_hash_final, NULL); // tzbsp_hash_final

    add_instrument(0x8879D150C, -1, tz_get_loglevel, NULL); // TZ EL1
    add_instrument(0x887A7A718, -1, retN, 1); // is_Anti_rollback_enabled
    add_instrument(0x887A3DC3C, -1, retN, 0); // some AC functionality

    add_instrument(0x14680000, -1, hook_qsee_start, NULL); // hook_qsee_start

    add_instrument(0x887A99790, -1, retN, 0); // print_smc
}