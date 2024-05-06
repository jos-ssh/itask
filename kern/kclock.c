/* See COPYRIGHT for copyright information. */

#include "inc/stdio.h"
#include "inc/vsyscall.h"
#include "kern/vsyscall.h"
#include <inc/x86.h>
#include <inc/time.h>
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

static long current_time = 0;
static bool periodic_enabled = false;

static int
get_time(void) {
    struct tm time;

    uint8_t s, m, h, d, M, y, Y, state;
    s = cmos_read8(RTC_SEC);
    m = cmos_read8(RTC_MIN);
    h = cmos_read8(RTC_HOUR);
    d = cmos_read8(RTC_DAY);
    M = cmos_read8(RTC_MON);
    y = cmos_read8(RTC_YEAR);
    Y = cmos_read8(RTC_YEAR_HIGH);
    state = cmos_read8(RTC_BREG);

    bool pm = false;
    if ((state & RTC_12H) == 0) {
      pm = true;
      h &= 0x7F;
    }

    if (!(state & RTC_BINARY)) {
        /* Fixup binary mode */
        s = BCD2BIN(s);
        m = BCD2BIN(m);
        h = BCD2BIN(h);
        d = BCD2BIN(d);
        M = BCD2BIN(M);
        y = BCD2BIN(y);
        Y = BCD2BIN(Y);
    }

    if (pm) {
      h += 12;
    }

    time.tm_sec = s;
    time.tm_min = m;
    time.tm_hour = h;
    time.tm_mday = d;
    time.tm_mon = M - 1;
    time.tm_year = y + Y * 100 - 1900;

    return timestamp(&time);
}

static void
rtc_timer_pic_interrupt(void) {
    uint8_t b_reg = cmos_read8(RTC_BREG);
    // Enable periodic interrupts
    cmos_write8(RTC_BREG, b_reg | RTC_PIE);

    uint8_t a_reg = cmos_read8(RTC_AREG);
    // Set rate
    cmos_write8(RTC_AREG, RTC_SET_NEW_RATE(a_reg, RTC_500MS_RATE));

    periodic_enabled = true;
}

static void
rtc_timer_pic_handle(void) {
    int status = rtc_check_status();
    // Clock updated, we have time to update timestamp
    if (status & RTC_UF) {
      current_time = get_time();
      vsys[VSYS_gettime] = current_time;
    }

    rtc_clear_status();
    pic_send_eoi(IRQ_CLOCK);

    // Periodic interrupt causes scheduling of next process
    if ((status & RTC_PF) && periodic_enabled) {
      sched_yield();
    }
}

struct Timer timer_rtc = {
        .timer_name = "rtc",
        .timer_init = rtc_timer_init,
        .enable_interrupts = rtc_timer_pic_interrupt,
        .handle_interrupts = rtc_timer_pic_handle,
};

int
gettime(void) {
    return current_time;
}

void
rtc_timer_init(void) {
    uint8_t b_reg = cmos_read8(RTC_BREG);
    if (!(b_reg & RTC_UIE))
      cmos_write8(RTC_BREG, b_reg | RTC_UIE);
    pic_irq_unmask(IRQ_CLOCK);
}

uint8_t
rtc_check_status(void) {
    return cmos_read8(RTC_CREG);
}

void rtc_clear_status(void) {
    cmos_write8(RTC_CREG, 0);
}
