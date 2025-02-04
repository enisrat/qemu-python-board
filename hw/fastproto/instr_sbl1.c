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

// Helper function to set X0 to 0
static void setX0_0(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = 0;
}

// Helper function to set X3 to 0
static void setX3_0(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[3] = 0;
}

// Helper function to set X10 to 0
static void setX10_0(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[10] = 0;
}

// ufs_overwrite_nonblocking function
static void ufs_overwrite_nonblocking(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[12] = 0;
    qemu_log_mask(LOG_TRACE, "Overwrite UFS NON_BLOCKING flag...\n");
}

// DALSysGetPropertyValue function
static void DALSysGetPropertyValue(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint8_t buffer[64];
    cpu_memory_rw_debug(cs, cpu->env.xregs[1], buffer, 64, false);
    char name[65];
    memcpy(name, buffer, 64);
    name[64] = '\0';
    qemu_log_mask(LOG_TRACE, "DALSysGetPropertyValue: %s id %llu\n", name, cpu->env.xregs[2]);
}

#define DDR_MAX_NUM_CH 8 //(why I dont know...)
typedef struct
{
  uint32_t ddr_cs0[DDR_MAX_NUM_CH];       /**< DDR size of Interface0 and chip select 0. IN MEGABYTES!!! */   
  uint32_t ddr_cs1[DDR_MAX_NUM_CH];       /**< DDR size of Interface0 and chip select 1. */
  
  uint64_t ddr_cs0_addr[DDR_MAX_NUM_CH];  /**< DDR start address of Interface0 and chip select 0. */
  uint64_t ddr_cs1_addr[DDR_MAX_NUM_CH];  /**< DDR start address of Interface0 and chip select 1. */
  
  uint32_t highest_bank_bit; /**< DDR Highest bank bit based on detected topology */

  uint32_t pad;
} ddr_size_info __attribute__((packed));

/** @brief ddr information that are relevent to clients outside of ddr driver */
typedef struct
{
  ddr_size_info ddr_size; /**< DDR size and address configuration */  
  uint32_t interleaved_memory; /**< Return whether the ddr is interleaved or not. */
  uint32_t ddr_type;  /**< Return ddr type enum. */ 
} ddr_info __attribute__((packed));

// ddr_initialize_info function
static void ddr_initialize_info(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    ddr_info my_ddr_info = {
        .ddr_size = {
            .ddr_cs0 = {2048, 2048, 0, 0, 0, 0, 0, 0},
            .ddr_cs1 = {2048, 2048, 0, 0, 0, 0, 0, 0},
            .ddr_cs0_addr = {(uint64_t)0x80000000, (uint64_t)0x100000000, 0, 0, 0, 0, 0, 0},
            .ddr_cs1_addr = {(uint64_t)0x180000000, (uint64_t)0x200000000, 0, 0, 0, 0, 0, 0},
            .highest_bank_bit = 0,
            .pad = 0
        },
        .interleaved_memory = 0,
        .ddr_type = 7
    };
    cpu_memory_rw_debug(cs, 0x14891978, &my_ddr_info, sizeof(my_ddr_info), true);
    cpu_memory_rw_debug(cs, 0x14891A48, &my_ddr_info, sizeof(my_ddr_info)-8, true);//ddr_system_size
    uint8_t init_done[] = {1};
    cpu_memory_rw_debug(cs, 0x14891970, init_done, sizeof(init_done), true);
    cpu->env.pc = cpu->env.xregs[30];
}

void sbl1_instrument()
{
    add_instrument(0x1485D5A8, -1, retN, 1); // a_lot_of_hw_init_sub_1485D5A8
    add_instrument(0x148371F8, -1, retN, 0); // some PLL init??
    add_instrument(0x1469F960, -1, retN, 1); // enable a PLL???
    add_instrument(0x14864298, -1, ufs_overwrite_nonblocking, NULL); // Fix a BUG in XBL not waiting for UFS command completion
    add_instrument(0x14850E30, -1, setX3_0, 0); // pmic_status = 0
    add_instrument(0x14850E8C, -1, setX10_0, 0);
    add_instrument(0x14850EAC, -1, setX0_0, 0);
    add_instrument(0x1484ECBC, -1, setX0_0, 0); // pmic_driver_init
    add_instrument(0x14850F34, -1, retN, 0); // usb_battery_check
    add_instrument(0x14837364, -1, retN, 0); // DDR stuff
    add_instrument(0x14837054, -1, retN, 1); // boot_pre_ddr_clock_initQQ 
    add_instrument(0x14848254, -1, DALSysGetPropertyValue, NULL);
    add_instrument(0x148243E8, -1, retN, 0); // do_ddr_training
    add_instrument(0x148C0000, -1, retN, 0); // boot_ddi_entry
    add_instrument(0x148245A8, -1, retN, 0); // sbl1_hw_init_secondary
    add_instrument(0x14837368, -1, retN, 0); // boot_populate_cpr_settings for SMEM
    add_instrument(0x14850D28, -1, setX0_0, 0); // pm_init_smem
    add_instrument(0x1483706C, -1, retN, 0); // boot_clock_init_rpm
    add_instrument(0x1482C4B8, -1, ddr_initialize_info, NULL); // boot_ddr_initialize_device
    add_instrument(0x1482D004, -1, retN, 0); // ddr_post_init
}