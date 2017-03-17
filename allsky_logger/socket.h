/*
 *                                                                                                  geany_encoding=koi8-r
 * socket.h
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
#pragma once
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "usefull_macros.h"
#include "cmdlnopts.h"

typedef enum{
    INTEGR, // int64_t
    DOUBLE  // double
} datatype;

typedef union{
    double d;
    int64_t i;
} bdata;

typedef struct{
    const char *varname;    // variable name as it comes from socket
    const char *keyname;    // FITS keyword name
    const char *comment;    // comment to FITS file
    datatype type;          // type of data
    bdata data;             // data itself
} datarecord;

void daemonize(glob_pars *G);
int watch_fits(char *name);

#endif // __SOCKET_H__
