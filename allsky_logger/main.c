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
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include "cmdlnopts.h"
#include "imfunctions.h"
#include "socket.h"

void signals(int signo){
    putlog("Main process died");
    exit(signo);
}

glob_pars *G; // global - for socket.c

int main(int argc, char **argv){
    initial_setup();
    signal(SIGTERM, signals); // kill (-15) - quit
    signal(SIGHUP, SIG_IGN);  // hup - ignore
    signal(SIGINT, signals);  // ctrl+C - quit
    signal(SIGQUIT, signals); // ctrl+\ - quit
    signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
    G = parse_args(argc, argv);
    if(!G->server) ERRX(_("Please, specify server name"));
    if(!G->filename) ERRX(_("Please, specify the name of input FITS file"));
    if(G->logfile) openlogfile(G->logfile);
    DBG("Opened, try to daemonize");
    daemonize();
    return 0;
}
