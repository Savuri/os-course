/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/trap.h>
#include <kern/picirq.h>


/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 *
 * Why it is necessary?
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here
    nmi_disable();
    // It is necessary because otherwise we will be working with nmi on ports 70, 71 not with cmos
    // You "select" a CMOS register (for reading or writing) by sending the register number to IO Port 0x70.
    // Since the 0x80 bit of Port 0x70 controls NMI, you always end up setting that, too.
    // So your CMOS controller always needs to know whether your OS wants NMI to be enabled or not.
    outb (IO_RTC_CMND, reg);
    uint8_t res = inb(IO_RTC_DATA);

    nmi_enable();

    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here
    nmi_disable();

    outb(IO_RTC_CMND, reg);
    outb(IO_RTC_DATA, value);

    nmi_enable();
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

void
rtc_timer_pic_interrupt(void) {
    // LAB 4: Your code here
    // Enable PIC interrupts.
    pic_irq_unmask(IRQ_CLOCK);
}

void
rtc_timer_pic_handle(void) {
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

void
rtc_timer_init(void) {
    // LAB 4: Your code here
    // (use cmos_read8/cmos_write8)
    uint8_t reg_a = cmos_read8(RTC_AREG);
    reg_a = reg_a | 0x0F; // 500 мс
    cmos_write8(RTC_AREG, reg_a);

    uint8_t reg_b = cmos_read8(RTC_BREG);
    reg_b |= RTC_PIE;
    cmos_write8(RTC_BREG, reg_b);
}

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8)

    // Determination that the RTC initiated an
    // interrupt is accomplished by reading Register C. A
    // logic '1' in the IRQF Bit indicates that one or more
    // interrupts have been initiated by the M48T86. The
    // act of reading Register C clears all active flag bits
    // and the IRQF Bit.


    return cmos_read8(RTC_CREG);
}
