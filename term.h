/*                                                                                                  geany_encoding=koi8-r
 * term.h - functions to work with serial terminal
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#pragma once
#ifndef __TERM_H__
#define __TERM_H__

#define FRAME_MAX_LENGTH        (300)
#define MAX_MEMORY_DUMP_SIZE    (0x800 * 4)
// terminal timeout (seconds)
#define     WAIT_TMOUT          (0.2)
// boltwood polling timeout - 15 seconds
#define     POLLING_TMOUT       (15.0)


// communication errors
typedef enum{
    TRANS_SUCCEED = 0,  // no errors
    TRANS_ERROR,        // some error occured
    TRANS_BADCHSUM,     // wrong checksum
    TRANS_TIMEOUT,      // no data for 0.1s
    TRANS_TRYAGAIN,     // checksum return 0x7f - maybe try again?
    TRANS_BUSY          // image exposure in progress
} trans_status;

typedef enum{
    PURGE_START = '!',
    MEMORY_DUMP_START = '|',
    BOOT_LOADER_START = '^',
    REQUEST_POLL = 0x01, /* SOH */
    FRAME_START = 0x02, /* STX */
    FRAME_END = '\n',
} term_symbols;

// main structure with useful data from cloud sensor
typedef struct{
    int humidstatTempCode;
    int rainCond;
    double skyMinusAmbientTemperature;
    double ambientTemperature;
    double windSpeed;
    int wetState; // 0 - dry, 1 - wet now, -1 - wet recently, -2 - unknown
    int relHumid;
    double dewPointTemperature;
    double caseTemperature;
    int rainHeaterState;
    double powerVoltage;
    double anemometerTemeratureDiff;
    int wetnessDrop;
    int wetnessAvg;
    int wetnessDry;
    int daylightADC;
    time_t tmsrment; // measurement time
} boltwood_data;


/******************************** Commands definition ********************************/
#define CMD_ACK         'a'
#define CMD_NACK        'n'

/******************************** Answers definition ********************************/
#define ANS_COMMAND_ACKED   'A'
#define ANS_COMMAND_NACKED  'N'
#define ANS_DATA_FRAME      'M'
#define ANS_POLLING_FRAME   'P'

/******************************** Data frame types ********************************/
#define DATAFRAME_SENSOR_REPORT     'D'

void run_terminal();
void try_connect(char *device);
int poll_sensor(boltwood_data *d);
int check_sensor();

#endif // __TERM_H__
