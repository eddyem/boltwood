/*                                                                                                  geany_encoding=koi8-r
 * client.c - simple terminal client
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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
#include "usefull_macros.h"
#include "term.h"
#include <strings.h> // strncasecmp
#include <time.h>    // time(NULL)

#define BUFLEN 1024

static uint16_t calcCRC16(const uint8_t *buf, int nBytes){
    /* Table generated using code written by Michael Barr
     * (http://www.netrino.com/Connecting/2000-01/crc.zip).
     */
    const uint16_t crc16Table[] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
    };

    uint16_t crc = 0;
    int i;
    for (i = 0 ; i < nBytes; ++i)
    crc = (uint16_t) ((crc << 8) ^ crc16Table[((crc >> 8) ^ buf[i]) & 0xff]);
    return crc;
}

/**
 * Send command by serial port, return 0 if all OK
 *static uint8_t last_chksum = 0;
int send_data(uint8_t *buf, int len){
    if(len < 1) return 1;
    uint8_t chksum = 0, *ptr = buf;
    int l;
    for(l = 0; l < len; ++l)
        chksum ^= ~(*ptr++) & 0x7f;
    DBG("send: %s (chksum: 0x%X)", buf, chksum);
    if(write_tty(buf, len)) return 1;
    DBG("cmd sent");
    if(write_tty(&chksum, 1)) return 1;
    DBG("checksum sent");
    last_chksum = chksum;
    return 0;
}*/

// send single symbol without CRC
int send_symbol(uint8_t cmd){
    uint8_t s[5] = {FRAME_START, cmd, '\n', REQUEST_POLL, 0};
    if(write_tty(s, 4)) return 1;
    //DBG("sent: %02x%s%02x", s[0], &s[1], s[3]);
    return 0;
}

/**
 * read string from terminal (ending with '\n') with timeout
 * @param str (o) - buffer for string
 * @param L       - its length
 * @return number of characters read
 */
static size_t read_string(uint8_t *str, int L){
    size_t r = 0, l;
    uint8_t *ptr = str;
    double d0 = dtime();
    do{
        if((l = read_tty(ptr, L))){
            r += l; L -= l; ptr += l;
            if(ptr[-1] == '\n') break;
            d0 = dtime();
        }
    }while(dtime() - d0 < WAIT_TMOUT);
    return r;
}

/**
 * Send command with CRC and ACK waiting
 * @return 0 if all OK
 */
int send_cmd(uint8_t cmd){
    uint8_t buf[BUFLEN];
    uint8_t s[10] = {FRAME_START, 'm', cmd,0};
    uint16_t crc = calcCRC16(&cmd, 1);
    snprintf((char*)&s[3], 6, "%04X\n", crc); // snprintf adds trailing zero
    s[8] = REQUEST_POLL;
    if(write_tty(s, 9)) return 1;
    int i, acked = 0;
    for(i = 0; i < 5; ++i){
        size_t L = read_string(buf, BUFLEN);
        if(L > 2){
            if(         (buf[0] == FRAME_START && buf[1] == ANS_COMMAND_ACKED) ||
                        (buf[1] == FRAME_START && buf[2] == ANS_COMMAND_ACKED)){
                acked = 1;
                break;
            }
        }
    }
    if(acked){
        //DBG("sent: %02x%s%02x", s[0], &s[1], s[8]);
        return 0;
    }
    return 1;
}

/**
 * Try to connect to `device` at given speed (or try all speeds, if speed == 0)
 * @return connection speed if success or 0
 */
int try_connect(char *device){
    if(!device) return 0;
    uint8_t tmpbuf[4096];
    green(_("Connecting to %s... "), device);
    fflush(stdout);
    tty_init(device);
    read_tty(tmpbuf, 4096); // clear rbuf
    green("Ok!");
    printf("\n\n");
    return 0;
}

// check CRC and return 0 if all OK
int chk_crc(uint8_t *buf){
    if(!buf) return 1;
   // DBG("CRC for %s", buf);
    uint8_t *strend = (uint8_t*) strchr((char*)buf, '\n');
    if(!strend) return 2;
    int L = (int) (strend - buf) - 4;
    if(L < 1) return 4; // string too short
    uint16_t calc = calcCRC16(buf, L);
    char *eptr;
    strend -= 4;
    unsigned long got = strtoul((char*)strend, &eptr, 16);
   // DBG("calc: 0x%x, got: 0x%lx", calc, got);
    if(eptr == (char*)strend || (*eptr && *eptr != '\n')) return 5; // bad input CRC
    if((uint16_t)got != calc) return 6; // not equal
    return 0;
}

/**
 * run terminal emulation: send user's commands with checksum and show answers
 */
void run_terminal(){
    green(_("Work in terminal mode without echo\n"));
    int rb;
    uint8_t usercmd = 0;
    uint8_t buf[BUFLEN], cmd;
    size_t L;
    setup_con();
    cmd = REQUEST_POLL;
    if(write_tty(&cmd, 1)) // start polling
        WARNX(_("can't request poll"));
    while(1){
        if((L = read_string(buf, BUFLEN))){
            if(L > 5 && buf[1] == ANS_DATA_FRAME){
                if(chk_crc(&buf[2])){
                    WARNX(_("Bad CRC in input data"));
                    send_symbol(CMD_NACK);
                    continue;
                }
                buf[L-5] = 0;
                send_symbol(CMD_ACK);
            }else if(L > 1){
                if(buf[1] == ANS_POLLING_FRAME || buf[2] == ANS_POLLING_FRAME){ // polling - send command or ack
                    if(usercmd){
                        if(!send_cmd(usercmd))
                            usercmd = 0;
                    }else
                        send_symbol(CMD_ACK);
                }
                continue;
            } else continue;
            // print out data frames
            uint8_t *ptr = &buf[1], c;
            while(L-- && (c = *ptr++)){
                if(c > 31) printf("%c", (char)c);
                else printf("[0x%02x]", c);
            }
            printf("\n");
        }
        if((rb = read_console())){
            if(rb > 31){
                if(send_cmd((uint8_t)rb))
                    usercmd = (uint8_t)rb;//send_cmd((uint8_t)rb);
            }else if(rb == '\n'){
                cmd = REQUEST_POLL;
                if(write_tty(&cmd, 1))
                    WARNX(_("can't request poll"));
            }
        }
    }
}

