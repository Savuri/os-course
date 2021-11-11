#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

int is_rsdp_check_sum_correct(uint8_t *pa, size_t length) {
    // Before the RSDP is relied upon you should check that the checksum is valid.
    // For ACPI 1.0 (the first structure) you add up every byte in the structure and make sure the lowest byte of the result is equal to zero.
    // For ACPI 2.0 and later you'd do exactly the same thing for the original (ACPI 1.0) part of the second structure, and then do it again for the fields that are part of the ACPI 2.0 extension.

    uint8_t res = 0;
    for (size_t i = 0; i < length; ++i) {
        res += pa[i];
    }

    return res == 0;
}

int is_header_check_sum_correct(ACPISDTHeader *tableHeader)
{
    unsigned char sum = 0;

    for (int i = 0; i < tableHeader->Length; ++i)
    {
        sum += ((uint8_t *) tableHeader)[i];
    }

    return sum == 0;
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */

    // LAB 5: Your code here
    static RSDT *rsdt = NULL;
    static size_t entry_elem_size;

    if (rsdt == NULL) {
        // find rsdt (extened rsdt == esdt)
        RSDP *rsdp = mmio_map_region(uefi_lp->ACPIRoot, sizeof(rsdp));
        // It was map for ACPI version > 1.0 (could have been overpowered map)


        // ACPI 1.0
        // size of struct is 20 bytes (In ACPI 1.0 there is no field length)
        assert(is_rsdp_check_sum_correct((uint8_t *)rsdp, 20));
        entry_elem_size = 4;
        uintptr_t rsdt_pa = rsdp->RsdtAddress;

        if (rsdp->Revision != 0) {
            // ACPI > 1.0
            assert(is_rsdp_check_sum_correct((uint8_t *)rsdp, rsdp->Length));
            entry_elem_size = 8;
            rsdt_pa = rsdp->XsdtAddress;
        }

        rsdt = mmio_map_region(rsdt_pa, sizeof(RSDT));
        assert(is_header_check_sum_correct(&rsdt->h));
        rsdt = mmio_map_region(rsdt_pa, rsdt->h.Length);
    }

    int entries_cnt = (rsdt->h.Length - sizeof(RSDT)) / entry_elem_size;

    for (int i = 0; i < entries_cnt; ++i) {
        uintptr_t addr;
        memcpy(&addr, (uint8_t *)rsdt->PointerToOtherSDT + i * entry_elem_size, entry_elem_size);

        ACPISDTHeader *h = (ACPISDTHeader *)mmio_map_region(addr, sizeof(ACPISDTHeader));
        h = mmio_map_region(addr, h->Length);

        assert(is_header_check_sum_correct(h));

        if (!strncmp(sign, h->Signature, sizeof(h->Signature))) {
            return h;
        }
    }


    return NULL;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names

    static FADT *kfadt;

    if (!kfadt) {
        kfadt = acpi_find_table("FACP");
    }

    return kfadt;
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)

    static HPET *khpet;

    if (!khpet) {
        khpet = (HPET *)acpi_find_table("HPET");
    }

    return khpet;
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        /* cprintf("hpetFemto = %llu\n", hpetFemto); */
        hpetFreq = (1 * Peta) / hpetFemto;
        /* cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega)); */
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    // LAB 5: Your code here
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF; // Enable LegacyReplacement Route
    hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF; // enable timer to cause an interrupt when the timer events fires
    hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF; // Writing a 1 to this bit enables the timer to generate a periodic interrupt.
    hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF; // By writing this bit to a 1, the software is then allowed to directly set a periodic timer’s accumulator
    hpetReg->TIM0_COMP = Peta / 2 / hpetFemto; // period of interrupt = 0.5 seconds
    pic_irq_unmask(IRQ_TIMER);
}

void
hpet_enable_interrupts_tim1(void) {
    // LAB 5: Your code here
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF; // Enable LegacyReplacement Route
    hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF; // enable timer to cause an interrupt when the timer events fires
    hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF; // Writing a 1 to this bit enables the timer to generate a periodic interrupt.
    hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF; // By writing this bit to a 1, the software is then allowed to directly set a periodic timer’s accumulator
    hpetReg->TIM1_COMP = 3 * Peta / 2 / hpetFemto; // period of interrupt = 1.5 seconds
    pic_irq_unmask(IRQ_CLOCK);
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    if (cpu_freq) {
        return cpu_freq;
    }

    const uint64_t part_of_second = 100;
    const uint64_t timer_tic = hpetFreq / part_of_second;
    uint64_t cnt_start = hpet_get_main_cnt();
    uint64_t tsc_start = read_tsc();

    while (hpet_get_main_cnt() - cnt_start < timer_tic) {
        asm("pause");
    }
    cpu_freq = (read_tsc() - tsc_start) * part_of_second;

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint32_t
sub24(uint32_t a, uint32_t b) {
    if (a > b) {
        return a - b;
    }
    if (b - a > 0xFFFFFF) {
        return 0xFFFFFFFF - b + a;
    }
    return 0xFFFFFF - b + a;
}

uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here

    if (cpu_freq) {
        return cpu_freq;
    }

    const uint64_t part_of_second = 100;
    uint64_t timer_tic = PM_FREQ / part_of_second;
    uint32_t pm_start = pmtimer_get_timeval();
    uint64_t tsc_start = read_tsc();

    while (sub24(pmtimer_get_timeval(), pm_start) < timer_tic) {
        asm("pause");
    }
    cpu_freq = (read_tsc() - tsc_start) * part_of_second;

    return cpu_freq;
}
