/*
 *                                                                                                  geany_encoding=koi8-r
 * socket.c - socket IO (both client & server)
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
#include "usefull_macros.h"
#include "socket.h"
#include "term.h"
#include <netdb.h>      // addrinfo
#include <arpa/inet.h>  // inet_ntop
#include <pthread.h>
#include <limits.h>     // INT_xxx
#include <signal.h> // pthread_kill
#include <unistd.h> // daemon
#include <sys/wait.h> // wait
#include <sys/prctl.h> //prctl

#define BUFLEN    (10240)
#define BUFLEN10  (1048576)
// Max amount of connections
#define BACKLOG   (30)

static uint64_t datctr = 0; // data portions counter
static boltwood_data boltwood; // global structure with last data

/**************** COMMON FUNCTIONS ****************/
/**
 * wait for answer from socket
 * @param sock - socket fd
 * @return 0 in case of error or timeout, 1 in case of socket ready
 */
static int waittoread(int sock){
    fd_set fds;
    struct timeval timeout;
    int rc;
    timeout.tv_sec = 1; // wait not more than 1 second
    timeout.tv_usec = 0;
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

/**************** SERVER FUNCTIONS ****************/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int send_data(int sock, int webquery){
    DBG("webq=%d", webquery);
    uint8_t buf[BUFLEN10], *bptr = buf, obuff[BUFLEN];
    ssize_t L;
    size_t rest = BUFLEN10, Len = 0;
    #define PUTI(val) do{L = snprintf((char*)bptr, rest, "%s=%d\n", #val, boltwood.val); \
                if(L > 0){rest -= L; bptr += L; Len += L;}}while(0)
    #define PUTD(val) do{L = snprintf((char*)bptr, rest, "%s=%.1f\n", #val, boltwood.val); \
                if(L > 0){rest -= L; bptr += L; Len += L;}}while(0)
    PUTI(humidstatTempCode);
    PUTI(rainCond);
    PUTD(skyMinusAmbientTemperature);
    PUTD(ambientTemperature);
    PUTD(windSpeed);
    PUTI(wetState);
    PUTI(relHumid);
    PUTD(dewPointTemperature);
    PUTD(caseTemperature);
    PUTI(rainHeaterState);
    PUTD(powerVoltage);
    PUTD(anemometerTemeratureDiff);
    PUTI(wetnessDrop);
    PUTI(wetnessAvg);
    PUTI(wetnessDry);
    PUTI(daylightADC);
    L = snprintf((char*)bptr, rest, "tmsrment=%ld\n", boltwood.tmsrment);
    if(L > 0){rest -= L; bptr += L; Len += L;}

    // OK buffer ready, prepare to send it
    if(webquery){
        L = snprintf((char*)obuff, BUFLEN,
            "HTTP/2.0 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Content-type: text/plain\r\nContent-Length: %zd\r\n\r\n", Len);
        if(L < 0){
            WARN("sprintf()");
            return 0;
        }
        if(L != write(sock, obuff, L)){
            WARN("write");
            return 0;
        }
        DBG("%s", obuff);
    }
    // send data
    red("send %zd bytes\n", Len);
    DBG("Buf: %s", buf);
    if(Len != (size_t)write(sock, buf, Len)){
        WARN("write()");
        return 0;
    }
    return 1;
}

// search a first word after needle without spaces
char* stringscan(char *str, char *needle){
    char *a, *e;
    char *end = str + strlen(str);
    a = strstr(str, needle);
    if(!a) return NULL;
    a += strlen(needle);
    while (a < end && (*a == ' ' || *a == '\r' || *a == '\t' || *a == '\r')) a++;
    if(a >= end) return NULL;
    e = strchr(a, ' ');
    if(e) *e = 0;
    return a;
}

void *handle_socket(void *asock){
    FNAME();
    int sock = *((int*)asock);
    int webquery = 0; // whether query is web or regular
    char buff[BUFLEN];
    uint64_t locctr = 0;
    ssize_t rd;
    while(1){
        if(!waittoread(sock)){ // no data incoming
            DBG("datctr:%lu, locctr: %lu", datctr, locctr);
            pthread_mutex_lock(&mutex);
            DBG("datctr:%lu, locctr: %lu", datctr, locctr);
            if(datctr != locctr){
                red("Send data, datctr = %ld, locctr = %ld\n", datctr, locctr);
                if(send_data(sock, webquery)){
                    locctr = datctr;
                    if(webquery){
                        pthread_mutex_unlock(&mutex);
                        break; // end of transmission
                    }
                }
            }
            pthread_mutex_unlock(&mutex);
            continue;
        }
        if(!(rd = read(sock, buff, BUFLEN-1))) continue;
        DBG("Got %zd bytes", rd);
        if(rd < 0){ // error or disconnect
            DBG("Nothing to read from fd %d (ret: %zd)", sock, rd);
            break;
        }
        // add trailing zero to be on the safe side
        buff[rd] = 0;
        // now we should check what do user want
        char *got, *found = buff;
        if((got = stringscan(buff, "GET")) || (got = stringscan(buff, "POST"))){ // web query
            webquery = 1;
            char *slash = strchr(got, '/');
            if(slash) found = slash + 1;
            // web query have format GET /some.resource
        }
        // here we can process user data
        printf("user send: %s\nfound=%s", buff,found);
    }
    close(sock);
    //DBG("closed");
    pthread_exit(NULL);
    return NULL;
}

void *server(void *asock){
    int sock = *((int*)asock);
    if(listen(sock, BACKLOG) == -1){
        WARN("listen");
        return NULL;
    }
    while(1){
        socklen_t size = sizeof(struct sockaddr_in);
        struct sockaddr_in their_addr;
        int newsock;
        if(!waittoread(sock)) continue;
        red("Got connection\n");
        newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
        if(newsock <= 0){
            WARN("accept()");
            continue;
        }
        pthread_t handler_thread;
        if(pthread_create(&handler_thread, NULL, handle_socket, (void*) &newsock))
            WARN("pthread_create()");
        else{
            DBG("Thread created, detouch");
            pthread_detach(handler_thread); // don't care about thread state
        }
    }
}

static void daemon_(int sock){
    FNAME();
    if(sock < 0) return;
    pthread_t sock_thread;
    if(pthread_create(&sock_thread, NULL, server, (void*) &sock))
        ERR("pthread_create()");
    do{
        if(pthread_kill(sock_thread, 0) == ESRCH){ // died
            WARNX("Sockets thread died");
            pthread_join(sock_thread, NULL);
            if(pthread_create(&sock_thread, NULL, server, (void*) &sock))
                ERR("pthread_create()");
        }
        // get data
        boltwood_data b;
        if(poll_sensor(&b) == 1){
            pthread_mutex_lock(&mutex);
            memcpy(&boltwood, &b, sizeof(boltwood_data));
            ++datctr;
            pthread_mutex_unlock(&mutex);
        }
        usleep(1000); // sleep a little or thread's won't be able to lock mutex
    }while(1);
}

/**
 * Run daemon service
 */
void daemonize(char *port){
    FNAME();
#ifndef EBUG
    green("Daemonize\n");
    if(daemon(1, 0)){
        ERR("daemon()");
    }
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
    }
#endif
    int sock = -1;
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, port, &hints, &res) != 0){
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
        int reuseaddr = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1){
            ERR("setsockopt");
        }
        if(bind(sock, p->ai_addr, p->ai_addrlen) == -1){
            close(sock);
            WARN("bind");
            continue;
        }
        break; // if we get here, we have a successfull connection
    }
    if(p == NULL){
        // looped off the end of the list with no successful bind
        ERRX("failed to bind socket");
    }
    freeaddrinfo(res);
    daemon_(sock);
    close(sock);
    signals(0);
}

