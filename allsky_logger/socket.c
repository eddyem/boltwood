/*
 *                                                                                                  geany_encoding=koi8-r
 * socket.c - socket IO
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
 *
 */
#include <netdb.h>      // addrinfo
#include <arpa/inet.h>  // inet_ntop
#include <limits.h>     // INT_xxx
#include <signal.h>     // pthread_kill
#include <unistd.h>     // daemon
#include <sys/wait.h>   // wait
#include <sys/prctl.h>  // prctl
#include <sys/inotify.h>// inotify_init1
#include <poll.h>       // pollfd

#include "usefull_macros.h"
#include "socket.h"
#include "imfunctions.h"

#define BUFLEN    (1024)
// Max amount of connections
#define BACKLOG   (30)

static double minstoragetime;

// keyname, comment, type, data
static datarecord boltwood_data[] = {
    {"humidstatTempCode"         , "HSCODE"  , "hum. and ambient temp. sensor code (0 - OK)"    , INTEGR, {0}},
    {"rainCond"                  , "RAINCOND", "rain sensor conditions (1 - no, 2 - rain)"      , INTEGR, {0}},
    {"skyMinusAmbientTemperature", "TSKYAMB" , "Tsky-Tamb (degrC)"                              , DOUBLE, {0}},
    {"ambientTemperature"        , "TAMB"    , "ambient temperature (degrC)"                    , DOUBLE, {0}},
    {"windSpeed"                 , "WIND"    , "wind speed (m/s)"                               , DOUBLE, {0}},
    {"wetState"                  , "WETSTATE", "wet sensor value (0 - dry, 1 - wet)"            , INTEGR, {0}},
    {"relHumid"                  , "HUMIDITY", "relative humidity in %"                         , INTEGR, {0}},
    {"dewPointTemperature"       , "DEWPOINT", "dew point temperature (degrC)"                  , DOUBLE, {0}},
    {"caseTemperature"           , "TCASE"   , "thermopile case temperature (degrC)"            , DOUBLE, {0}},
    {"rainHeaterState"           , "RHTRSTAT", "state of rain sensor heater (1 - OK)"           , INTEGR, {0}},
    {"powerVoltage"              , "PWRVOLT" , "power voltage value (V)"                        , DOUBLE, {0}},
    {"anemometerTemeratureDiff"  , "ANEMTDIF", "anemometer tip temperature difference(degrC)"   , DOUBLE, {0}},
    {"wetnessDrop"               , "WETDROP" , "rain drops count for this cycle"                , INTEGR, {0}},
    {"wetnessAvg"                , "WETAWG"  , "wetness osc. count diff. from base dry value"   , INTEGR, {0}},
    {"wetnessDry"                , "WETDRY"  , "wetness osc. dry count diff. from base value"   , INTEGR, {0}},
    {"daylightADC"               , "DAYADC"  , "daylight photodiode raw A/D output"             , INTEGR, {0}},
    {"tmsrment"                  , "TBOLTWD" , "Boltwood's sensor measurement time (UNIX TIME)" , INTEGR, {0}},
    {NULL, NULL, NULL, 0, {0}}
};

/**
 * Find parameter `par` in pairs "parameter=value"
 * if found, return pointer to `value`
 */
static char *findpar(const char *str, const char *par){
    size_t L = strlen(par);
    char *f = strstr((char*)str, par);
    if(!f) return NULL;
    f += L;
    if(*f != '=') return NULL;
    return (f + 1);
}
/**
 * get integer & double `parameter` value from string `str`, put value to `ret`
 * @return 1 if all OK
 */
static int getintpar(const char *str, const char *parameter, int64_t *ret){
    int64_t tmp;
    char *endptr;
    if(!(str = findpar(str, parameter))) return 0;
    tmp = strtol(str, &endptr, 0);
    if(endptr == str || *str == 0 )
        return 0;
    if(ret) *ret = tmp;
    return 1;
}
static int getdpar(const char *str, const char *parameter, double *ret){
    double tmp;
    char *endptr;
    if(!(str = findpar(str, parameter))) return 0;
    tmp = strtod(str, &endptr);
    if(endptr == str || *str == 0)
        return 0;
    if(ret) *ret = tmp;
    return 1;
}

/**
 * Boltwood's sensor data parser
 * @param buf  (i) - zero-terminated text buffer with data received
 * @param data (o) - boltwood data (allocated here if *data == NULL) - unchanged if input buffer wrong
 */
static void parse_data(char *buf, datarecord **data){
    if(!data) return;
    if(!*data){ // copy boltwood_data[] array
        *data = MALLOC(datarecord, sizeof(boltwood_data));
        datarecord *rec = boltwood_data, *optr = *data;
        while(rec->varname){
            optr->varname = strdup(rec->varname);
            optr->keyname = strdup(rec->keyname);
            optr->comment = strdup(rec->comment);
            optr->type = rec->type;
            ++rec; ++optr;
        }
    }
    datarecord *rec = boltwood_data;
    int status = 1; // 1 - good, 0 - bad
    while(rec->varname){
        switch(rec->type){
            case INTEGR:
                if(!getintpar(buf, rec->varname, &rec->data.i)){
                    status = 0;
                    goto ewhile;
                }
            break;
            case DOUBLE:
            default:
                if(!getdpar(buf, rec->varname, &rec->data.d)){
                    status = 0;
                    goto ewhile;
                }
        }
        ++rec;
    }
ewhile:
    if(status){ // all fields received - copy them
        rec = boltwood_data;
        datarecord *optr = *data;
        while(rec->varname){
            optr->data = rec->data;
            ++optr; ++rec;
        }
    }
}

/**
 * wait for answer from socket
 * @param sock - socket fd
 * @return 0 in case of error or timeout, 1 in case of socket ready
 */
static int waittoread(int sock){
    fd_set fds;
    struct timeval timeout;
    int rc;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // wait not more than 100ms
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    do{
        rc = select(sock+1, &fds, NULL, NULL, &timeout);
        if(rc < 0){
            if(errno != EINTR){
                WARN("select()");
                return 0;
            }
            continue;
        }
        break;
    }while(1);
    if(FD_ISSET(sock, &fds)) return 1;
    return 0;
}

/**
 * Open given FITS file, check it and add to inotify
 * THREAD UNSAFE!!!
 * @return inotify file descriptor
 */
int watch_fits(char *name){
    static int fd = -1, wd = -1;
    if(fd > 0 && wd > 0){
        inotify_rm_watch(fd, wd);
        wd = -1;
    }
    if(!test_fits(name)) ERRX(_("File %s is not FITS file with 2D image!"), name);
    if(fd < 0)
        fd = inotify_init1(IN_NONBLOCK);
    if(fd == -1) ERR("inotify_init1()");
    FILE* f = fopen(name, "r");
    if(!f) ERR("fopen()"); // WTF???
    fclose(f);
    if((wd = inotify_add_watch(fd, name, IN_CLOSE_WRITE)) < 0)
        ERR("inotify_add_watch()");
    DBG("file %s added to inotify", name);
    return fd;
}

/**
 * test if user can write something to path & make CWD
 */
static void test_path(char *path){
    if(path){
        if(chdir(path)) ERR("Can't chdir(%s)", path);
    }
    if(access("./", W_OK)) ERR("access()");
    DBG("OK, user can write to given path");
}

/**
 * Client daemon itself
 * @param FITSpath - path to file watched
 * @param infd - inotify file descriptor
 * @param sock - socket's file descriptor
 */
static void client_(char *FITSpath, int infd, int sock){
    if(sock < 0) return;
    size_t Bufsiz = BUFLEN;
    char *recvBuff = MALLOC(char, Bufsiz);
    datarecord *last_good_msrment = NULL;
    int poll_num;
    struct pollfd fds;
    fds.fd = infd;
    fds.events = POLLIN;
    char buf[256];
    double lastTstorage = -1.; // last time of file saving
    void rlc(size_t newsz){
        if(newsz >= Bufsiz){
            Bufsiz += 1024;
            recvBuff = realloc(recvBuff, Bufsiz);
            if(!recvBuff){
                WARN("realloc()");
                return;
            }
            DBG("Buffer reallocated, new size: %zd\n", Bufsiz);
        }
    }
    DBG("Start polling");
    while(1){
        poll_num = poll(&fds, 1, 1);
        if(poll_num == -1){
           if (errno == EINTR)
               continue;
           ERR("poll()");
           return;
        }
        if(poll_num > 0){
            DBG("changed?");
            if(fds.revents & POLLIN){
                ssize_t len = read(infd, &buf, sizeof(buf));
                if (len == -1 && errno != EAGAIN) {
                   ERR("read");
                   return;
                }else{
                    DBG("file changed");
                    usleep(100000); // wait a little for file changes
                    fds.fd = watch_fits(FITSpath);
                    if(dtime() - lastTstorage > minstoragetime){
                        DBG("lastT: %.2g, now: %.2g", lastTstorage, dtime());
                        lastTstorage = dtime();
                        store_fits(FITSpath, last_good_msrment);
                    }
                }
            }
        }
        if(!waittoread(sock)) continue;
        size_t offset = 0;
        do{
            rlc(offset);
            ssize_t n = read(sock, &recvBuff[offset], Bufsiz - offset);
            if(!n) break;
            if(n < 0){
                WARN("read");
                break;
            }
            offset += n;
        }while(waittoread(sock));
        if(!offset){
            WARN("Socket closed\n");
            return;
        }
        rlc(offset);
        recvBuff[offset] = 0;
        DBG("read %zd bytes", offset);
        parse_data(recvBuff, &last_good_msrment);
    }
}

/**
 * Connect to socket and run daemon service
 */
void daemonize(glob_pars *G){
    char resolved_path[PATH_MAX];
    // get full path to FITS file
    if(!realpath(G->filename, resolved_path)) ERR("realpath()");
    // open FITS file & add it to inotify
    int fd = watch_fits(G->filename);
    // CD to archive directory if user wants
    test_path(G->cwd);
    minstoragetime = G->min_storage_time;
    // run fork before socket opening to prevent daemon's death if there's no network
    #ifndef EBUG
    green("Daemonize\n");
    if(daemon(1, 0)){
        ERR("daemon()");
    while(1){ // guard for dead processes
        pid_t childpid = fork();
        if(childpid){
            DBG("Created child with PID %d\n", childpid);
            wait(NULL);
            WARNX("Child %d died\n", childpid);
            sleep(1);
        }else{
            prctl(PR_SET_PDEATHSIG, SIGTERM); // send SIGTERM to child when parent dies
            break; // go out to normal functional
        }
    }}
    #endif
    int sock = -1;
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    DBG("connect to %s:%s", G->server, G->port);
    if(getaddrinfo(G->server, G->port, &hints, &res) != 0){
        ERR("getaddrinfo");
    }
    struct sockaddr_in *ia = (struct sockaddr_in*)res->ai_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ia->sin_addr), str, INET_ADDRSTRLEN);
    DBG("canonname: %s, port: %u, addr: %s\n", res->ai_canonname, ntohs(ia->sin_port), str);
    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next){
        if((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            WARN("socket");
            continue;
        }
        if(connect(sock, p->ai_addr, p->ai_addrlen) == -1){
            WARN("connect()");
            close(sock);
            continue;
        }
        break; // if we get here, we have a successfull connection
    }
    if(p == NULL){
        // looped off the end of the list with no successful bind
        ERRX("failed to bind socket");
    }
    freeaddrinfo(res);
    client_(resolved_path, fd, sock);
    close(sock);
    signals(0);
}
