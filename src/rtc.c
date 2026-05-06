#include "rtc.h"
#include <gb/gb.h>

/*
 * MBC3 register access via memory-mapped writes to ROM space.
 * The MBC intercepts writes to 0x0000-0x7FFF; reads/writes to
 * 0xA000-0xBFFF hit whichever bank/register is currently selected.
 */
#define MBC3_RAM_ENABLE  (*(volatile uint8_t*)0x0000u)
#define MBC3_BANK_SEL    (*(volatile uint8_t*)0x4000u)
#define MBC3_LATCH       (*(volatile uint8_t*)0x6000u)
#define MBC3_DATA        (*(volatile uint8_t*)0xA000u)

#define RTC_REG_S   0x08u
#define RTC_REG_M   0x09u
#define RTC_REG_H   0x0Au
#define RTC_REG_DL  0x0Bu   /* days low byte */
#define RTC_REG_DH  0x0Cu   /* days high + flags */

static void mbc3_enable(void)  { MBC3_RAM_ENABLE = 0x0Au; }
static void mbc3_disable(void) { MBC3_RAM_ENABLE = 0x00u; }

static void mbc3_latch_clock(void) {
    MBC3_LATCH = 0x00u;
    MBC3_LATCH = 0x01u;
}

static uint8_t mbc3_rtc_read_reg(uint8_t reg) {
    MBC3_BANK_SEL = reg;
    return MBC3_DATA;
}

static void mbc3_rtc_write_reg(uint8_t reg, uint8_t val) {
    MBC3_BANK_SEL = reg;
    MBC3_DATA     = val;
}

void rtc_read(RTCTime *t) {
    uint8_t dh;
    mbc3_enable();
    mbc3_latch_clock();
    t->seconds = mbc3_rtc_read_reg(RTC_REG_S) & 0x3Fu;
    t->minutes = mbc3_rtc_read_reg(RTC_REG_M) & 0x3Fu;
    t->hours   = mbc3_rtc_read_reg(RTC_REG_H) & 0x1Fu;
    t->days    = mbc3_rtc_read_reg(RTC_REG_DL);
    dh         = mbc3_rtc_read_reg(RTC_REG_DH);
    t->days   |= (uint16_t)((dh & 0x01u) << 8u);
    mbc3_disable();
}

void rtc_write(const RTCTime *t) {
    uint8_t dh;
    mbc3_enable();
    /* Halt the clock before writing (bit 6 of DH register) */
    mbc3_rtc_write_reg(RTC_REG_DH, 0x40u);
    mbc3_rtc_write_reg(RTC_REG_S,  t->seconds);
    mbc3_rtc_write_reg(RTC_REG_M,  t->minutes);
    mbc3_rtc_write_reg(RTC_REG_H,  t->hours);
    mbc3_rtc_write_reg(RTC_REG_DL, (uint8_t)(t->days & 0xFFu));
    dh = (uint8_t)((t->days >> 8u) & 0x01u);  /* clear halt bit to restart */
    mbc3_rtc_write_reg(RTC_REG_DH, dh);
    mbc3_disable();
}

void rtc_reset(void) {
    RTCTime zero = {0, 0, 0, 0};
    rtc_write(&zero);
}

uint32_t rtc_elapsed_seconds(void) {
    RTCTime t;
    rtc_read(&t);
    return (uint32_t)t.days  * 86400UL
         + (uint32_t)t.hours *  3600UL
         + (uint32_t)t.minutes *  60UL
         + (uint32_t)t.seconds;
}
