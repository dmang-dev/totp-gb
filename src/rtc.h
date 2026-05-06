#ifndef RTC_H
#define RTC_H

#include <stdint.h>

/*
 * MBC3 Real-Time Clock interface.
 *
 * The MBC3 RTC tracks elapsed time as days/hours/minutes/seconds.
 * We store a base Unix timestamp in SRAM and add the RTC elapsed
 * seconds to get the current Unix time.
 */

typedef struct {
    uint8_t seconds;  /* 0-59  */
    uint8_t minutes;  /* 0-59  */
    uint8_t hours;    /* 0-23  */
    uint16_t days;    /* 0-511 */
} RTCTime;

/* Read latched RTC registers into t */
void rtc_read(RTCTime *t);

/* Write values into RTC registers and restart counting */
void rtc_write(const RTCTime *t);

/* Reset RTC to 0 (call when user sets a new base epoch) */
void rtc_reset(void);

/* Return elapsed seconds since last rtc_reset() */
uint32_t rtc_elapsed_seconds(void);

#endif
