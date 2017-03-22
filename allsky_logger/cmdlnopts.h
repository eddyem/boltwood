/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.h - comand line options for parceargs
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

#pragma once
#ifndef __CMDLNOPTS_H__
#define __CMDLNOPTS_H__

#include "parseargs.h"
#include "term.h"

/*
 * here are some typedef's for global data
 */
typedef struct{
    char *port;             // port to connect
    char *cwd;              // change working directory to given
    char *server;           // server name
    char *filename;         // input file name
    double min_storage_time;// minimal storage period
} glob_pars;


glob_pars *parse_args(int argc, char **argv);
#endif // __CMDLNOPTS_H__
