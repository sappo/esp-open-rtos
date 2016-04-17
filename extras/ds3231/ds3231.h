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

#ifndef __DS3231_H__
#define __DS3231_H__

#include <time.h>

/*  Alarm flags  */
#define DS3231_ALARM_NONE 0
#define DS3231_ALARM_1    1
#define DS3231_ALARM_2    2
#define DS3231_ALARM_BOTH 3

#define DS3231_ALARM1_EVERY_SECOND         0
#define DS3231_ALARM1_MATCH_SEC            1
#define DS3231_ALARM1_MATCH_SECMIN         2
#define DS3231_ALARM1_MATCH_SECMINHOUR     3
#define DS3231_ALARM1_MATCH_SECMINHOURDAY  4
#define DS3231_ALARM1_MATCH_SECMINHOURDATE 5

#define DS3231_ALARM2_EVERY_MIN         0
#define DS3231_ALARM2_MATCH_MIN         1
#define DS3231_ALARM2_MATCH_MINHOUR     2
#define DS3231_ALARM2_MATCH_MINHOURDAY  3
#define DS3231_ALARM2_MATCH_MINHOURDATE 4

bool ds3231_getTime(struct tm *time);
bool ds3231_setTime(struct tm *time);

bool ds3231_enableAlarmInts(uint8_t alarms);
bool ds3231_disableAlarmInts(uint8_t alarms);
bool ds3231_clearAlarmFlags(uint8_t alarm);
bool ds3231_getAlarmFlags(uint8_t *alarms);
bool ds3231_setAlarm(uint8_t alarms, struct tm *time1, uint8_t option1,
                                     struct tm *time3, uint8_t option2);

bool ds3231_getTempInteger(int8_t *temp);
bool ds3231_getTempBytes(int8_t *tempMSB, int8_t *tempLSB);
bool ds3231_getTempFloat(float *temp);

bool ds3231_enable32khz();
bool ds3231_disable32khz();
bool ds3231_enableSquarewave();
bool ds3231_disableSquarewave();
bool ds3232_setSquarewaveFreq(uint8_t freq);

bool ds3231_getOscillatorStopFlag(bool *flag);
bool ds3231_clearOscillatorStopFlag();

#endif
