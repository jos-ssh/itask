/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/timer.h>
#include <kern/trap.h>
#include <kern/picirq.h>
#include <kern/sched.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 * NOTE: CMOS_CMD is the same port that is used to toggle NMIs,
 * so nmi_disable() cannot be used. And you have to use provided
 * constant.
 *
 * Why it is necessary?
 * The CMOS RTC expects a read from or write to the data port 0x71 after any
 * write to index port 0x70 or it may go into an undefined state. This means
 * that we cannot write to CMOS_CMD twice in a row and must select the register
 * at the same time as disabling NMI
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */

    // Select register 'reg' and disable NMI
    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);
    // Read data from selected register
    uint8_t res = inb(CMOS_DATA);
    // Re-enable NMI
    nmi_enable();
    // Read and discard data (required for CMOS RTC to work)
    inb(CMOS_DATA);
    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here
    // Select register 'reg' and disable NMI
    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);
    // Write data to selected register
    outb(CMOS_DATA, value);
    // Re-enable NMI
    nmi_enable();
    // Read and discard data (required for CMOS RTC to work)
    inb(CMOS_DATA);
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

static void
rtc_timer_pic_interrupt(void) {
    pic_irq_unmask(IRQ_CLOCK);
}

static void
rtc_timer_pic_handle(void) {
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);

    sched_yield();
}

struct Timer timer_rtc = {
        .timer_name = "rtc",
        .timer_init = rtc_timer_init,
        .enable_interrupts = rtc_timer_pic_interrupt,
        .handle_interrupts = rtc_timer_pic_handle,
};

void
rtc_timer_init(void) {
    uint8_t b_reg = cmos_read8(RTC_BREG);
    if (!(b_reg & RTC_PIE)) {
        cmos_write8(RTC_BREG, b_reg | RTC_PIE);
    }
    uint8_t a_reg = cmos_read8(RTC_AREG);
    cmos_write8(RTC_AREG, RTC_SET_NEW_RATE(a_reg, RTC_500MS_RATE));
}

uint8_t
rtc_check_status(void) {
    return cmos_read8(RTC_CREG);
}
