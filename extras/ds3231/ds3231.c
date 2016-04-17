/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.

 * For more information, please refer to <http://unlicense.org/>
 */

#include <stdio.h>
#include <stdlib.h>
#include <i2c/i2c.h>
#include "ds3231.h"

/*  Defines  */

#define DS3231_ADDR 0x68

#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ      0x08
#define DS3231_STAT_BUSY       0x04
#define DS3231_STAT_ALARM_2    0x02
#define DS3231_STAT_ALARM_1    0x01

#define DS3231_CTRL_OSCILLATOR    0x80
#define DS3231_CTRL_SQUAREWAVE_BB 0x40
#define DS3231_CTRL_TEMPCONV      0x20
#define DS3231_CTRL_SQWAVE_4096HZ 0x10
#define DS3231_CTRL_SQWAVE_1024HZ 0x08
#define DS3231_CTRL_SQWAVE_8192HZ 0x18
#define DS3231_CTRL_SQWAVE_1HZ    0x00
#define DS3231_CTRL_ALARM_INTS    0x04
#define DS3231_CTRL_ALARM2_INT    0x02
#define DS3231_CTRL_ALARM1_INT    0x01

#define DS3231_ALARM_WDAY   0x40
#define DS3231_ALARM_NOTSET 0x80

#define DS3231_ADDR_TIME    0x00
#define DS3231_ADDR_ALARM1  0x07
#define DS3231_ADDR_ALARM2  0x0b
#define DS3231_ADDR_CONTROL 0x0e
#define DS3231_ADDR_STATUS  0x0f
#define DS3231_ADDR_AGING   0x10
#define DS3231_ADDR_TEMP    0x11

#define DS3231_SET     0
#define DS3231_CLEAR   1
#define DS3231_REPLACE 2

#define DS3231_12HOUR_FLAG 0x40
#define DS3231_12HOUR_MASK 0x1f
#define DS3231_PM_FLAG     0x20
#define DS3231_MONTH_MASK  0x1f

/*  Local helper functions  */

/*  Convert normal decimal to binary coded decimal  */

static uint8_t s_dec_to_bcd (uint8_t dec)
{
    return (((dec / 10) * 16) + (dec % 10));
}


/*  Convert binary coded decimal to normal decimal  */

static uint8_t s_bcd_to_dec (uint8_t bcd)
{
    return (((bcd / 16) * 10) + (bcd % 16));
}


/*
 *  Send a number of bytes to the RTC over i2c. Returns true if successful
 *  false otherwise.
 */

static bool s_ds3231_send (uint8_t *data, uint8_t len)
{
    return i2c_slave_write(DS3231_ADDR, data, len);
}


/*
 *  Read a number of bytes from the RTC over i2c. Returns true if successful
 *  otherwise false.
 */

static bool s_ds3231_recv (uint8_t addr, uint8_t *data, uint8_t len)
{
    return i2c_slave_read(DS3231_ADDR, addr, data, len);
}


/*
 *  Set or clear bits in a byte register, or replace the byte altogether pass the
 *  register address to modify, a byte to replace the existing value with or
 *  containing the bits to set/clear and one of:
 *    * DS3231_SET
 *    * DS3231_CLEAR
 *    * DS3231_REPLACE
 *  Returns true if successful, otherwise false.
 */

bool s_ds3231_set_flag(uint8_t addr, uint8_t bits, uint8_t mode)
{
    uint8_t data[2];
    data[0] = addr;
    /*  Get status register  */
    if (s_ds3231_recv (*data, data + 1, 1)) {
        /*  Clear the flag  */
        if (mode == DS3231_REPLACE) data[1] = bits;
        else if (mode == DS3231_SET) data[1] |= bits;
        else data[1] &= ~bits;
        if (s_ds3231_send(data, 2)) {
            return true;
        }
    }
    return false;
}


/*
 *  Sets the time on the RTC. Timezone agnostic, pass whatever you like. I
 *  suggest using GMT and applying timezone and DST when read back. Returns true
 *  if successful, otherwise false.
 */

bool ds3231_setTime(struct tm *time) {
    uint8_t data[8];
    // start register
    data[0] = DS3231_ADDR_TIME;
    // time/date data
    data[1] = s_dec_to_bcd(time->tm_sec);
    data[2] = s_dec_to_bcd(time->tm_min);
    data[3] = s_dec_to_bcd(time->tm_hour);
    data[4] = s_dec_to_bcd(time->tm_wday + 1);
    data[5] = s_dec_to_bcd(time->tm_mday);
    data[6] = s_dec_to_bcd(time->tm_mon + 1);
    data[7] = s_dec_to_bcd(time->tm_year - 100);

    return s_ds3231_send(data, 8);

}


/*
 *  Set alarms:
 *    * Alarm 1 works with seconds, minutes, hours and day of week/month, or
 *      fires every second.
 *    * Alarm 2 works with          minutes, hours and day of week/month, or
 *      fires every minute.
 *  Not all combinations are supported, see DS3231_ALARM1_* and DS3231_ALARM2_*
 *  defines for valid options you only need to populate the fields you are using
 *  in the tm struct, and you can set both alarms at the same time (pass
 *  DS3231_ALARM_1, DS3231_ALARM_2, DS3231_ALARM_BOTH) if only setting one alarm
 *  just pass 0 for time1 or time2 and option field for the other alarm if using
 *  DS3231_ALARM1_EVERY_SECOND or DS3231_ALARM2_EVERY_MIN you can pass 0 for tm
 *  struct if you want to enable interrupts for the alarms you need to do that
 *  separately. Returns true if successful, otherwise false.
 */

bool ds3231_setAlarm(uint8_t alarms, struct tm *time1, uint8_t option1,
                                     struct tm *time2, uint8_t option2)
{
    int index = 0;
    uint8_t data[8];

    /*  Start register  */
    data[index++] = (alarms == DS3231_ALARM_2 ? DS3231_ADDR_ALARM2 : DS3231_ADDR_ALARM1);

    /* Alarm 1 data */
    if (alarms != DS3231_ALARM_1) {
        data[index++] = (option1 >= DS3231_ALARM1_MATCH_SEC ?
                        s_dec_to_bcd(time1->tm_sec) : DS3231_ALARM_NOTSET);
        data[index++] = (option1 >= DS3231_ALARM1_MATCH_SECMIN ?
                        s_dec_to_bcd(time1->tm_min) : DS3231_ALARM_NOTSET);
        data[index++] = (option1 >= DS3231_ALARM1_MATCH_SECMINHOUR ?
                        s_dec_to_bcd(time1->tm_hour) : DS3231_ALARM_NOTSET);
        data[index++] = (option1 == DS3231_ALARM1_MATCH_SECMINHOURDAY ?
                        (s_dec_to_bcd(time1->tm_wday + 1) & DS3231_ALARM_WDAY) :
                            (option1 == DS3231_ALARM1_MATCH_SECMINHOURDATE ?
                                s_dec_to_bcd(time1->tm_mday) : DS3231_ALARM_NOTSET));
    }

    /*  Alarm 2 data  */
    if (alarms != DS3231_ALARM_2) {
        data[index++] = (option2 >= DS3231_ALARM2_MATCH_MIN ?
            s_dec_to_bcd(time2->tm_min) : DS3231_ALARM_NOTSET);
        data[index++] = (option2 >= DS3231_ALARM2_MATCH_MINHOUR ?
            s_dec_to_bcd(time2->tm_hour) : DS3231_ALARM_NOTSET);
        data[index++] = (option2 == DS3231_ALARM2_MATCH_MINHOURDAY ?
            (s_dec_to_bcd(time2->tm_wday + 1) & DS3231_ALARM_WDAY) :
                (option2 == DS3231_ALARM2_MATCH_MINHOURDATE ?
                    s_dec_to_bcd(time2->tm_mday) : DS3231_ALARM_NOTSET));
    }

    return s_ds3231_send(data, index);
}


/*
 *  Get a byte containing just the requested bits pass the register address to
 *  read, a mask to apply to the register and an uint* for the output you can
 *  test this value directly as true/false for specific bit mask of use a mask
 *  of 0xff to just return the whole register byte. Returns successful,
 *  otherwise false.
 */

bool ds3231_getFlag (uint8_t addr, uint8_t mask, uint8_t *flag)
{
    uint8_t data[1];
    if (s_ds3231_recv (addr, data, 1)) {
        /*  Return only requested flag  */
        *flag = (data[0] & mask);
        return true;
    }
    return false;
}


/*
 *  Check if oscillator has previously stopped, e.g. no power/battery or disabled
 *  sets flag to true if there has been a stop. Returns true if successful,
 *  otherwise false.
 */

bool ds3231_getOscillatorStopFlag(bool *flag)
{
    uint8_t f;
    if (ds3231_getFlag(DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, &f)) {
        *flag = (f ? true : false);
        return true;
    }
    return false;
}


/*
 *  Clear the oscillator stopped flag. Returns true if successful, otherwise
 *  false.
 */

bool ds3231_clearOscillatorStopFlag()
{
    return s_ds3231_set_flag(DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, DS3231_CLEAR);
}


/*
 *  Check which alarm(s) have past sets alarms to DS3231_ALARM_NONE/
 *  DS3231_ALARM_1/DS3231_ALARM_2/DS3231_ALARM_BOTH. Returns true if successful,
 *  otherwise false.
 */

bool ds3231_getAlarmFlags(uint8_t *alarms)
{
    return ds3231_getFlag(DS3231_ADDR_STATUS, DS3231_ALARM_BOTH, alarms);
}

/*
 *  Clear alarm past flag(s). Pass DS3231_ALARM_1/DS3231_ALARM_2/
 *  DS3231_ALARM_BOTH. Returns true successful otherwise false.
 */

bool ds3231_clearAlarmFlags(uint8_t alarms)
{
    return s_ds3231_set_flag(DS3231_ADDR_STATUS, alarms, DS3231_CLEAR);
}

/*
 *  Enable alarm interrupts and thereby disable squarewave. Pass DS3231_ALARM_1,
 *  DS3231_ALARM_2, DS3231_ALARM_BOTH. If you set only one alarm the status of
 *  the other is not changed. You must also clear any alarm past flag(s) for
 *  alarms with interrupt enabled, else it will trigger immediately. Returns true
 *  if successful, otherwise false.
 */

bool ds3231_enableAlarmInts(uint8_t alarms)
{
    return s_ds3231_set_flag(DS3231_ADDR_CONTROL,
                          DS3231_CTRL_ALARM_INTS | alarms, DS3231_SET);
}


/*
 *  Disable alarm interrupts (does not (re-)enable squarewave). Pass
 *  DS3231_ALARM_1/DS3231_ALARM_2/DS3231_ALARM_BOTH. Returns true successful,
 *  otherwise false.
 */

bool ds3231_disableAlarmInts(uint8_t alarms)
{
    /*  Just disable specific alarm(s) requested  */
    /*  Doesn't disable alarm interrupts generally, which would enable squarewave  */
    return s_ds3231_set_flag(DS3231_ADDR_CONTROL, alarms, DS3231_CLEAR);
}


/*
 *  Enable the output of 32kHz signal. Returns true if successful, otherwise
 *  false.
 */

bool ds3231_enable32khz()
{
    return s_ds3231_set_flag(DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_SET);
}


/*
 *  Disable the output of 32kHz signal. Returns true if successful, otherwise,
 *  false.
 */

bool ds3231_disable32khz()
{
    return s_ds3231_set_flag(DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_CLEAR);
}


/*
 *  Enable the squarewave output (disables alarm interrupt functionality).
 *  Returns true if successful, otherwise false.
 */

bool ds3231_enableSquarewave()
{
    return s_ds3231_set_flag(DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_CLEAR);
}


/*
 *  Disable the squarewave output (which re-enables alarm interrupts, but
 *  individual alarm interrupts also need to be enabled, if not already,
 *  before they will trigger). Returns true if successful, otherwise false.
 */

bool ds3231_disableSquarewave()
{
    return s_ds3231_set_flag(DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_SET);
}


/*
 *  Set the frequency of the squarewave output (but does not enable it) pass
 *  DS3231_SQUAREWAVE_RATE_1HZ/DS3231_SQUAREWAVE_RATE_1024HZ/
 *  DS3231_SQUAREWAVE_RATE_4096HZ/DS3231_SQUAREWAVE_RATE_8192HZ. Returns true
 *  if successful, otherwise false.
 */

bool ds3231_setSquarewaveFreq(uint8_t freq)
{
    uint8_t flag = 0;
    if (ds3231_getFlag(DS3231_ADDR_CONTROL, 0xff, &flag)) {
        /*  Clear current rate  */
        flag &= ~DS3231_CTRL_SQWAVE_8192HZ;
        /*  Set new rate  */
        flag |= freq;
        return s_ds3231_set_flag(DS3231_ADDR_CONTROL, flag, DS3231_REPLACE);
    }
    return false;
}


/*
 *  Get the temperature as an integer (rounded down). Returns true successful,
 *  otherwise false.
 */

bool ds3231_getTempInteger(int8_t *temp)
{
    uint8_t data[1];
    /*  Get just the integer part of the temp  */
    if (s_ds3231_recv (DS3231_ADDR_TEMP, data, 1)) {
        *temp = (signed)data[0];
        return true;
    }
    return false;
}


/*
 *  Get the temperature as two bytes; the whole number integer part and the
 *  fractional part.  The fractional part is encoded in two bits of the second
 *  byte (so precision is limited to 1/4 of a degree C). Returns true if
 *  successful, otherwise false.
 */

bool ds3231_getTempBytes(int8_t *tempMSB, int8_t *tempLSB)
{
    uint8_t data[2];
    /*  Get just the integer part of the temp  */
    if (s_ds3231_recv (DS3231_ADDR_TEMP, data, 2)) {
        *tempMSB = ((signed)data[0]);
        *tempLSB = ((unsigned)((data[1]) >> 6) * 0.25) * 100;
        return true;
    }
    return false;
}


/*
 *  Get the temperature as a float (in quarter degree increments). Returns true
 *  successful, otherwise false.
 */

bool ds3231_getTempFloat(float *temp)
{
    uint8_t data[2];
    /*  Get integer part and quarters of the temp  */
    if (s_ds3231_recv (DS3231_ADDR_TEMP, data, 2)) {
        *temp = ((signed)data[0]) + (((unsigned)(data[1]) >> 6) * 0.25);
        return true;
    }
    return false;
}


/*
 *  Get the time from the RTC, populates a supplied tm struct. Returns true if
 *  successful, otherwise false.
 */

bool ds3231_getTime(struct tm *time)
{
    uint8_t data[7];
    /*  Read time  */
    if (!s_ds3231_recv (DS3231_ADDR_TIME, data, 7)) {
        return false;
    }

    /*  Convert to UNIX time structure  */
    time->tm_sec = s_bcd_to_dec(data[0]);
    time->tm_min = s_bcd_to_dec(data[1]);
    if (data[2] & DS3231_12HOUR_FLAG) {
        /*  12h  */
        time->tm_hour = s_bcd_to_dec(data[2] & DS3231_12HOUR_MASK);
        /*  pm?  */
        if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
    } else {
        /*  24h  */
        time->tm_hour = s_bcd_to_dec(data[2]);
    }
    time->tm_wday = s_bcd_to_dec(data[3]) - 1;
    time->tm_mday = s_bcd_to_dec(data[4]);
    time->tm_mon  = s_bcd_to_dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = s_bcd_to_dec(data[6]) + 100;
    time->tm_isdst = 0;

    /*  Apply a time zone (if you are not using localtime on the rtc or you  */
    /*  want to check/apply DST) applyTZ(time);  */
    return true;
}
