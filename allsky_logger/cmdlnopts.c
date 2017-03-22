/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.c - the only function that parse cmdln args and returns glob parameters
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "cmdlnopts.h"
#include "usefull_macros.h"

/*
 * here are global parameters initialisation
 */
int help;
glob_pars  G;

#define DEFAULT_COMDEV  "/dev/ttyUSB0"
//            DEFAULTS
// default global parameters
glob_pars const Gdefault = {
    .cwd = NULL,            // current working directory
    .port = "55555",        // port to listen boltwood data
    .server = NULL,         // server name
    .filename = NULL,       // input file name
    .min_storage_time = 60.,// minimal storage period
};

/*
 * Define command line options by filling structure:
 *  name        has_arg     flag    val     type        argptr              help
*/
myoption cmdlnopts[] = {
// common options
    {"help",    NO_ARGS,    NULL,   'h',    arg_int,    APTR(&help),        _("show this help")},
    {"port",    NEED_ARG,   NULL,   'p',    arg_string, APTR(&G.port),      _("port to connect (default: 55555)")},
    {"address", NEED_ARG,   NULL,   'a',    arg_string, APTR(&G.server),    _("server name")},
    {"filename",NEED_ARG,   NULL,   'i',    arg_string, APTR(&G.filename),  _("input FITS file name")},
    {"mindt",   NEED_ARG,   NULL,   't',    arg_double, APTR(&G.min_storage_time), _("minimal time interval (in seconds) for image storage (default: 60s)")},
    end_option
};

/**
 * Parse command line options and return dynamically allocated structure
 *      to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parse_args(int argc, char **argv){
    int i;
    void *ptr;
    ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
    // format of help: "Usage: progname [args]\n"
    change_helpstring(_("Usage: %s [args] [working dir]\n\n\tWhere args are:\n"));
    // parse arguments
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(argc > 0){ // the rest should be working directory
        if(argc != 1){
            red(_("Too many arguments:\n"));
            for (i = 0; i < argc; i++)
                printf("\t%s\n", argv[i]);
            printf(_("Should be path to archive directory\n"));
            signals(1);
        }
        G.cwd = argv[0];
    }
    return &G;
}

