/*                                                                                                  geany_encoding=koi8-r
 * main.c
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
#include "usefull_macros.h"
#include <signal.h>
#include "cmdlnopts.h"
#include "socket.h"

void signals(int signo){
    restore_console();
    restore_tty();
    exit(signo);
}

int main(int argc, char **argv){
    initial_setup();
    signal(SIGTERM, signals); // kill (-15) - quit
    signal(SIGHUP, SIG_IGN);  // hup - ignore
    signal(SIGINT, signals);  // ctrl+C - quit
    signal(SIGQUIT, signals); // ctrl+\ - quit
    signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
    glob_pars *G = parse_args(argc, argv);
    if(G->rest_pars_num)
        openlogfile(G->rest_pars[0]);
    #ifndef EBUG
    if(daemon(1, 0)){
        ERR("daemon()");
    }
    #endif
    try_connect(G->device);
    if(check_sensor()) signals(15); // there's not Boltwood sensor connected
    if(G->terminal) run_terminal();
    else daemonize(G->port);
    return 0;
}
