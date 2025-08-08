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

static void add_instruments_for_UEFI_image();

static void retN(CPUState *cs, vaddr pc, void *opaque)
{
    qemu_log_mask(LOG_TRACE, "HIT instrument @%llx cpu %d %llx\n", pc, cs->cpu_index, opaque);
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.xregs[0] = (uint64_t)opaque;
    cpu->env.pc = cpu->env.xregs[30];
}

static void serial_write_buffered(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint8_t one = 1;
    cpu_memory_rw_debug(cs, 0x9FC38B44, &one, 1, true);  // set SyncIO_enabled flag
    qemu_log_mask(LOG_TRACE, "Serial write\n");
}

static void disable_mmu_before_ttbr0_set(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.cp15.sctlr_ns &= 0xFFFFFFFFFFFFFFFE; // disable MMU
    tlb_flush(cs);
    arm_rebuild_hflags(&cpu->env);
}

static void set_rpmh_is_standalone(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint8_t one = 1;
    cpu_memory_rw_debug(cs, 0xA505B5F8, &one, sizeof(one), true);
}

// Structure to hold hex key and ascii value
typedef struct {
    char key[33]; // 32 for the hex + 1 for null terminator
    char value[100]; // Assuming max length of value is less than 100
} guid_name_pair;

// Static array of guid_name_pair structures
static guid_name_pair guid_to_name[] = {
{"4d3a82cdec7d3145ae5d4134fa4127b8", "UsbConfigDxe"},
{"5acc8186f60d1e44b4b8e915c538f067", "DALTLMM"},
{"1e04ea495267ca42b0b17344fe2546b7", "ArmTimerDxe"},
{"79c780f77cddcd47bd1a5eb414c51704", "BATTERY.PROVISION"},
{"2e2376572d08754b9a0efe1d13f7a5d9", "PmicDxe"},
{"15e85347d8dd2d40bf699b8c4eb7573e", "battery_symbol_nobattery.bmp"},
{"6357286438fcba418edc5c171f108235", "Panel_sharp_qsync_fhd_vid.xml"},
{"7fcba2d6186a2f4eb43b9920a733700a", "DxeCore"},
{"b6af3bcdfb50e84f8e4eab74d2c1a600", "EnglishDxe"},
{"59a83c258f1d6c468ea458163bd0d550", "Panel_s6e3hc2_1080p_dsc_cmd.xml"},
{"d8e33cd9eba730478c8ecc466a9ecc3c", "RscRtDxe"},
{"cab85c5a82930c4eb38377fb517cd9eb", "AdcDxe"},
{"db81d15b87041a4fae73820e165611b3", "ButtonsDxe"},
{"f4260f7f9481c84d8c0fb77aaa1443c7", "CitadelTransportSpiDxe"},
{"cecd0e2bae016e449ffdc4217de0340f", "VariableDxe"},
{"cd8ab519f8b17840b9a5a33584b680e2", "Panel_sharp_4k_dsc_vid.xml"},
{"bf5aca88acd09340a68c0cfae1ef9635", "Panel_boe_amoled_wqhd_dsc_cmd.xml"},
{"681ec732a883ed46aed1094e71b12057", "SecRSADxe"},
{"7e77d9b82ad71f459bdbbafb52a68415", "ArmCpuDxe"},
{"df93e1106699e744b17c59dd831e20fc", "ChipInfo"},
{"40b0e795a22611459abb1d95d6da7082", "Panel_truly_wqxga_dual_vid.xml"},
{"1b477e7c9bd2064aadcc1e0563278097", "Panel_sofef00_1080p_cmd.xml"},
{"c99dac1392007443ae1a06ff35950fd0", "Panel_truly_wqxga_dsc_vid.xml"},
{"20c9f98ce6d5ac4dbef96e6a4eec7add", "PILProxyDxe"},
{"75b4d38b1a400b4b9315edee61a1eae5", "VcsDxe"},
{"c0257190f1a5e311a3fea3198b49350c", "FvSimpleFileSystem"},
{"f81fa0abcb2c124e8b2ecd3f4a742993", "CmdDbDxe"},
{"1ca6e47973ed124394fee3e7563362a9", "PrintDxe"},
{"f5a4f9c2b4f7e743ba995ea804cc103a", "ASN1X509Dxe"},
{"63d641f5484aaa40aabfff158ccae34c", "SmemDxe"},
{"850617d76559444b930537cdb199b9be", "Panel_boe_amoled_fhd_dsc_cmd.xml"},
{"d1f429cb377f9246a41693e82e219766", "RpmhDxe"},
{"30b80104f9bfa244a5f695734a33c017", "PwrUtilsDxe"},
{"7f3198f8ce88394889edb80e94e4cbbc", "Panel_sharp_qsync_wqhd_cmd.xml"},
{"a7a6f89434dc014188c199179cceae83", "UsbfnDwc3Dxe"},
{"7c1f37dec4de214dadf1593abcc15882", "ArmGicDxe"},
{"b77de13ac53c894b93049549211057ef", "ResetRuntimeDxe"},
{"8ecd350dea979a4f96af0f0d89f76567", "UFSDxe"},
{"c4f801b6b743844795b1f4226cb40cee", "RuntimeDxe"},
{"d9bde9213f6c104f84a5bbec322741f1", "uefipil.cfg"},
{"624d8c34bdbf82489ecec80bb1c4783b", "HiiDatabase"},
{"e3837a2d43e3fb4f9109631f2ee11694", "ScmDxe"},
{"44de5e6df86fa543920210de8e574a86", "SpiIoDxe"},
{"b4f7386b98ade9409093aca2b5a253c4", "DiskIoDxe"},
{"d623e30abe2ee81183499ba636a0d80c", "Panel_sharp_1080p_cmd.xml"},
{"f342302eb92b3f46886656d75b7c4ab0", "ChargerExDxe"},
{"d3fa256943e2694bad613f978c8abc60", "TzDxe"},
{"8d13521ba33f504eb95820887353f809", "CPRDxe"},
{"107ff75adf907e4e8325a17ec09d5443", "UsbMsdDxe"},
{"beeb1507877c604b9422503d144fd36c", "ShipModeDxe"},
{"29f5041c120619439315b2e75c16810a", "FeatureEnablerDxe"},
{"244a53e29a1d544aaaecbe8836d0f45d", "FvDxe"},
{"9e68d3b0f811c6438ece023a29cec35b", "RngDxe"},
{"1b2105b1bdbbdd4aa3b0d1cf4a52154c", "PlatformInfoDxeDriver"},
{"b1c2e9eeca16344f87ea2e6d1e160cc4", "QcomChargerApp"},
{"cd37095ff9735e44b320bb6fd5d3cbde", "CipherDxe"},
{"ef4b7b2acd80e149b473374ba4d673fc", "SPMI"},
{"e1a5436377e42a45b62c1560c4cbd9f5", "Panel_secondary_truly_1080p_cmd.xml"},
{"b795c58b1a399a4a82565b9983b30392", "Panel_sharp_4k_dsc_cmd.xml"},
{"7bba2876f098fa4ba62894c8d1e94f40", "Panel_s6e3hc2_evt3_1080p_dsc_cmd.xml"},
{"db223cb433630c49872d0a73439059fd", "PdcDxe"},
{"99f3cc51df4f554ea45be123f84d456a", "ConPlatformDxe"},
{"ef04206c0e4ee44bb14c340eb4aa5891", "SCHandlerRtDxe"},
{"8e831da9faa53841825d455e2303079e", "BDS_Menu.cfg"},
{"ffacff0bf269cf4281ca73973f1bac23", "FuseInfoDxe"},
{"9185de04b3d27740bbbeb12070094eb6", "I2C"},
{"81aa505aaec30846a0e341a2e69baf94", "QcomBds"},
{"67026e4c7dc70d4181001495911a989d", "MetronomeDxe"},
{"7c4bfe450c15da45a0214beb2048ec6f", "QcomChargerCfg.cfg"},
{"8e831da9faa53841825d455e23030794", "logo1.bmp"},
{"66a29932f01546438318716336736d3e", "UsbDeviceDxe"},
{"367fce4b8e42934399e37e0844404dba", "QcomChargerDxeLA"},
{"da8ddf3a5018c5448c63bb991849bc6f", "HashDxe"},
{"4cedfa111fb2884d8e48c4c28a1e50df", "UsbPwrCtrlDxe"},
{"db760ff1c1423f5334a869be24653110", "SdccDxe"},
{"6357286438fcba418edc5c171f108229", "Panel_sharp_qsync_fhd_cmd.xml"},
{"552cae8c5afc0b41812b8ca22f572f36", "Panel_truly_wqxga_dsc_cmd.xml"},
{"3477311d8340c64783eae3a42f2a6bff", "PILDxe"},
{"d1f429cb377f9246a41693e82e219711", "NpaDxe"},
{"280ccbcc244bd5119a5a0090273fc14d", "GraphicsConsoleDxe"},
{"837179337f1cd8498ccd4474e08b8f40", "Panel_sharp_wqhd_dualdsi_cmd.xml"},
{"2f023841c706794f9c947e33b511a4e7", "DisplayDxe"},
{"a6deb54d02531a4d8a82677a683b0d29", "ClockDxe"},
{"9ef3a11ffffeae4abd7b38a070a3b609", "PartitionDxe"},
{"97ec4754d93c364fb653c7a2178158c7", "Panel_sharp_wqhd_dualdsi_vid.xml"},
{"c9a52365cb91c045a2dc25997cf23369", "Panel_boe_amoled_wqhd_dsc_vid.xml"},
{"423b27871d31e8119324dfb2dadfc3d1", "Panel_truly_1080p_cmd.xml"},
{"1fb71e9c6cddd54e9f6a5cc0ca789f16", "ShmBridgeDxe"},
{"df09fb7d7b2a9f409dbe38e7662ead9d", "NfcWakeControlDxe"},
{"a05b7937cfe1d54ebd8b16df6bcb1e1a", "EmbeddedMonotonicCounter"},
{"ce0f689b6bad3a4fb60bf59899003443", "DevicePathDxe"},
{"bdab65cf41904b4684f42acd1ac44a0d", "Panel_sharp_qsync_wqhd_vid.xml"},
{"2ed99af05c48db9c3e053385c38f94cc", "TsensDxe"},
{"a26397af3b0309418e1756a98d380c92", "HWIODxeDriver"},
{"d5ff15ed96bc0741911895366600188e", "FontDxe"},
{"dea09e5657b5434084cf01103fe516e5", "GpiDxe"},
{"0779d93ff1934943af3c3b68b0a5e626", "battery_symbol_charger.bmp"},
{"f91563fcfef82242a1e3226df55d7592", "Panel_truly_wqxga_dual_cmd.xml"},
{"611e0e04fb0a1b419ec900f3fc59cc13", "QcomWDogDxe"},
{"9c86a43b1553e8119c2dfa7ae01bbebc", "battery_symbol_lowbattery.bmp"},
{"2ed99af0914e0f49ab99e69939b840b2", "LimitsDxe"},
{"7fd699f0ae71364cb2a3dceb0eb2b7d8", "WatchdogTimer"},
{"de35e2a125e891459623c43175811826", "SecParti.cfg"},
{"39585fa2554d8f428f0b5ce1d565f53e", "VerifiedBootDxe"},
{"70d313a7ed01a5419b678c700f7427a3", "Panel_s6e3hc2_dvt_1080p_dsc_cmd.xml"},
{"a82831e49286b6428afa676158578d18", "ULogDxe"},
{"9101090b4ca9d14c9fbee648a8cac97f", "Panel_sofef01_1080p_cmd.xml"},
{"fd9aa4902f42ae089611e788d3804845", "EnvDxe"},
{"ecdc8e406dcf7c47a5a8b4844e3de281", "ConSplitterDxe"},
{"41877a85ec0ebd43948227d14ed47d72", "SimpleTextInOutSerial"},
{"225d1c157f956148a605e27154bbda25", "Panel_secondary_truly_1080p_vid.xml"},
{"1f77009ad436d54d8916c48ed9b16b86", "HALIOMMU"},
{"3894d3534a819c4d87c107e8c713f4c4", "QcomMpmTimerDxe"},
{"0a7f8542f213214b8a2353d3f714b840", "CapsuleRuntimeDxe"},
{"60d61b7a85a1924f9003cc71d22ad121", "ADSPDxe"},
{"fe781596b7b6c344af356bc705cd2b1f", "Fat"},
{"0032fdba1d31e81186510fb3cacf38fb", "Panel_truly_1080p_vid.xml"},
{"60d19b8e84b1df1194e20800200c9a66", "DALSys"},
{"2df636b33541554aae4e4971bbf0885d", "RealTimeClock"},
{"e99706f8d67f6546864688e33ef71dfc", "SecurityStubDxe"},
{"663a5820263c9e4d9a8ce2dfa2914015", "MacDxe"},
{"3a79b07d0244e14b906ed0fabad2707e", "DDRInfoDxe"},
{"c13200e91b678342a9a3bd225f3b3bcd", "Roboto-Regular.ttf"},
{"2d7f49baefb5494bb1cb3c01e19b9b19", "RedWarning1080.bmp"},
{"e1ea408dbdae764c892962d04d3e0882", "LinuxLoader"},
{"2d6fa586cef6a045b906a24a95c8106d", "Splash1080.bmp"},
{"985d72ffd521794e80bc82c71defec81", "FastbootTransportUsbDxe"},
{"45c4ba695aa13548957f94214b8b09db", "Splash1440.bmp"},
{"96e05422c2428b4f8f30641a95663be8", "YellowWarning1440.bmp"},
{"3dd4b2d92fe0864aa445bd1474b33671", "RedWarning1440.bmp"},
{"2069f96122c02b4baa801c364a4f64b1", "B1c1FastbootApp"},
{"9ab3acae93656147a4b38b8aa5b0d19c", "OrangeWarning1080.bmp"},
{"f25f89603ed0994d8d1a46af496fdd31", "Power1080.bmp"},
{"a6876b15fa17b94f9589141f4bf2428b", "SplashDark1080.bmp"},
{"b1dab920d2905e49af04769954265da9", "VolumeUp1080.bmp"},
{"503bcd037465294fb6a1510a7d765127", "Power1440.bmp"},
{"353356e7b5062e44ba2de0db99895a8a", "OrangeWarning1440.bmp"},
{"3c7f0801831be34b94ab58081b5346da", "VolumeDown1080.bmp"},
{"3a7d1149bda36a458a45557c5bd0116c", "YellowWarning1080.bmp"},
{"52fabaeea5b91f46a2bbbc3bd2969cd4", "SplashDark1440.bmp"},
{"1b66084221a7834f9fd8b8b85b4802ff", "VolumeUp1440.bmp"},
{"fecf087a818aa74cad12480bdb70c2c9", "VolumeDown1440.bmp"},
};

// Function to convert bytes to hex string
void bytes_to_hex(char* hex_str, uint8_t* bytes, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        sprintf(hex_str + (i * 2), "%02x", bytes[i]);
    }
}
// Function to search for a value given a guid
char* find_value_by_guid(uint8_t guid[16]) {
    char hex_key[33]; // 32 for the hex string + 1 for null terminator
    bytes_to_hex(hex_key, guid, 16);

    // Search through the array
    for (int i = 0; i < (sizeof(guid_to_name) / sizeof(guid_to_name[0])); i++)
    {
        if (strcmp(guid_to_name[i].key, hex_key) == 0) {
            return guid_to_name[i].value;
        }
    }
    return NULL; // Not found
}

static char *last_image_loaded = NULL;
static uint64_t last_image_base = 0;

static void CoreImageLoad(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t hwpath_ptr = cpu->env.xregs[2];
    char buffer[512];
    cpu_memory_rw_debug(cs, hwpath_ptr, buffer, 512, false);

    int i = 0;
    uint8_t type;
	uint8_t guid[16];
    while (i < 512) {
        type = *(uint8_t *)(buffer + i);
        if (type == 0x7f) break;

        // Read subtype and length
        uint16_t length;
        memcpy(&length, buffer + i + 2, sizeof(length));

        // Print GUID
        memcpy(guid, buffer + i + 4, 16);

        i += length;
    }

	char *found = find_value_by_guid(guid);
    last_image_loaded = found;
	if(found) {
		qemu_log_mask(LOG_TRACE, "CoreImageLoad: %s\n", found);
    }
    else
    {
        qemu_log_mask(LOG_TRACE, "CoreImageLoad: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
				guid[0], guid[1], guid[2], guid[3],
				guid[4], guid[5], guid[6], guid[7],
				guid[8], guid[9], guid[10], guid[11],
				guid[12], guid[13], guid[14], guid[15]);
    }
}


static void CoreImageLoad_DestAddrDetermined(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint64_t dest_addr = cpu->env.xregs[0];
    qemu_log_mask(LOG_TRACE, "DestAddr: %llx\n", dest_addr);

    last_image_base = dest_addr;
    if(last_image_loaded) add_instruments_for_UEFI_image();
}

static void QseeLoadServiceImage(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint32_t uuid;
    cpu_memory_rw_debug(cs, cpu->env.xregs[1], &uuid, 4, false);
    qemu_log_mask(LOG_TRACE, "QseeLoadServiceImage: %08X\n", uuid);
}

static void set_CPU_BYPASS_RANGE_CHECKING(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint32_t one = 1;
    cpu_memory_rw_debug(cs, 0xA50354C0, &one, 4, true);
}

//break simple endless loops
static void loop_breaker(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    cpu->env.pc += 4;
}

static void PlatformInfo_Init_Struct(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint32_t platform = 0xf; //EFI_PLATFORMINFO_TYPE_RUMI
    // X19+0x10 -> PlatformInfo Struct
    cpu_memory_rw_debug(cs, cpu->env.xregs[19]+0x10, &platform, 4, true); //set "platform" field
}

// do not use RUMI Platform Type for UFS, because it would trigger a lot of additional
// HW code inside UFSDxe... (probably for testing UFS HW)
static void UFS_set_g_platform(CPUState *cs, vaddr pc, void *opaque)
{
    ARMCPU *cpu = ARM_CPU(cs);
    uint32_t platform = 8;
    cpu->env.xregs[8] = platform; 
}

static void add_instruments_for_UEFI_image(){
    if (strcmp(last_image_loaded, "RpmhDxe") == 0) 
    {
        add_instrument(last_image_base+0x3734, -1, set_rpmh_is_standalone, 0); //rpmh_client_init
    } 
    else if (strcmp(last_image_loaded, "PdcDxe") == 0)
    {
        add_instrument(last_image_base+0x23A8, -1, retN, 0); // PDCDxe pdc_seq_handle_init
        add_instrument(last_image_base+0x2418, -1, retN, 0); // PDCDxe pdc_seq_enable
    }
    else if (strcmp(last_image_loaded, "UFSDxe") == 0)
    {
        add_instrument(last_image_base+0x252C, -1, retN, 0); // UFSDxe UFSSmmuConfig
        add_instrument(last_image_base + 0x1373C, -1, UFS_set_g_platform, 0); //during ufs_bsp_get_platform_config
    }
    else if (strcmp(last_image_loaded, "TzDxe") == 0)
    {
        add_instrument(last_image_base+0x26E4, -1, QseeLoadServiceImage, 0);
    }
    else if (strcmp(last_image_loaded, "ClockDxe") == 0)
    {
        add_instrument(last_image_base+0xB3B4, -1, set_CPU_BYPASS_RANGE_CHECKING, 0); //Clock_SetCpuPerfLevel
        add_instrument(last_image_base + 0xBF5C, -1, loop_breaker, 0);
        add_instrument(last_image_base + 0x11B78, -1, loop_breaker, 0);
        add_instrument(last_image_base + 0x5D8C, -1, retN, 0); //clock_SourceOn
    }
    else  if (strcmp(last_image_loaded, "PmicDxe") == 0)
    {
        //add_instrument(last_image_base+0x10ec8, -1, retN, 1); //PmicInitializeDetect
    }
    else  if (strcmp(last_image_loaded, "PlatformInfoDxeDriver") == 0)
    {
        // Set Pltform to RUMI. Makes some emulation easier, as the UEFI modules check this occasionally 
        // and are then aware that PMIC is not funtional
        add_instrument(last_image_base+0x206C, -1, PlatformInfo_Init_Struct, 0); //PlatformInfo_Init_Struct (after pDrvCtxt->pGlbCtxt was copied)
    }else  if (strcmp(last_image_loaded, "CPRDxe") == 0)
    {
        add_instrument(last_image_base+0x145C, -1, retN, 0); //CPRDxeEntryPoint
    }
    else /* default: */
    {
    }
}

void xbl_uefi_instrument()
{
    add_instrument(0x9FC13E88, -1, serial_write_buffered, 0);
    add_instrument(0x9FC17760, -1, disable_mmu_before_ttbr0_set, 0);

    add_instrument(0xA51ADE90, -1, CoreImageLoad, 0);
    add_instrument(0xA51ADA8C, -1, CoreImageLoad_DestAddrDetermined, 0);
}
