/*
 *                                                                                                  geany_encoding=koi8-r
 * imfunctions.c - functions to work with image
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
#include <strings.h> // strncasecmp
#include <math.h>    // sqrt
#include <time.h>    // time, gmtime etc
#include <fitsio.h>  // save fits
#include <libgen.h>  // basename
#include <sys/stat.h>// utimensat
#include <fcntl.h>   // AT_...
#include <fitsio.h>
#include <sys/sendfile.h> // sendfile

#include "usefull_macros.h"
#include "imfunctions.h"
#include "term.h"
#include "socket.h"

/**
 * All image-storing functions modify ctime of saved files to be the time of
 * exposition start!
 */
void modifytimestamp(char *filename, time_t tv_sec){
    if(!filename) return;
    struct timespec times[2];
    memset(times, 0, 2*sizeof(struct timespec));
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = tv_sec; // change mtime
    if(utimensat(AT_FDCWD, filename, times, 0)) WARN(_("Can't change timestamp for %s"), filename);
}

/**
 * Test if `name` is a fits file with 2D image
 * @return 1 if all OK
 */
int test_fits(char *name){
    fitsfile *fptr;
    int status = 0, nfound, ret = 0;
    long naxes[2];
    if(fits_open_file(&fptr, name, READONLY, &status)){
        goto retn;
    }
    // test image size
    if(fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status)){
        goto retn;
    }
    if(naxes[0] > 0 && naxes[1] > 0) ret = 1;
retn:
    if(status) fits_report_error(stderr, status);
    fits_close_file(fptr, &status);
    return ret;
}


/**
 * copy FITS file with absolute path 'oldname' to 'newname' in current dir (using sendfile)
 * @return 0 if all OK
 *
static int copyfile(char *oldname, char *newname){
    struct stat st;
    int ret = 1, ifd = -1, ofd = -1;
    if(stat(oldname, &st) || !S_ISREG(st.st_mode)){
        WARN("stat()");
        goto rtn;
    }
    ifd = open(oldname, O_RDONLY);
    ofd = open(newname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH); // rewrite existing files
    if(ifd < 0 || ofd < 0){
        WARN("open()");
        goto rtn;
    }
    posix_fadvise(ifd, 0, 0, POSIX_FADV_SEQUENTIAL); // tell kernel that we want to copy file -> sequential
    posix_fadvise(ofd, 0, 0, POSIX_FADV_SEQUENTIAL);
    ssize_t bytes = sendfile(ofd, ifd, NULL, st.st_size);
    if(bytes < 0){
        WARN("sendfile()");
        goto rtn;
    }
    if(bytes == st.st_size) ret = 0;
    DBG("copy %s -> %s %ssuccesfull", oldname, newname, ret ? "un" : "");
rtn:
    if(ifd > 0) close(ifd);
    if(ofd > 0) close(ofd);
    return ret;
}*/

static void mymkdir(char *name){
    if(mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
        if(errno != EEXIST)
            ERR("mkdir()");
    }
    DBG("mkdir(%s)", name);
}

static void gotodir(char *relpath){ // create directory structure
    if(chdir(relpath)){ // can't chdir -> test
        if(errno != ENOENT) ERR("Can't chdir");
    }else return;
    // no such directory -> create it
    char *p = relpath;
    for(; *p; ++p){
        if(*p == '/'){
            *p = 0;
            mymkdir(relpath);
            *p = '/';
        }
    }
    mymkdir(relpath);
    if(-1 == chdir(relpath)) ERR("chdir()");
}

#define TRYFITS(f, ...)                     \
do{ int status = 0;                         \
    f(__VA_ARGS__, &status);                \
    if (status){                            \
        fits_report_error(stderr, status);  \
        goto ret;}                          \
}while(0)
#define WRITEIREC(k, v, c)                                   \
do{ int status = 0;                                          \
    snprintf(record, FLEN_CARD, "%-8s=%21ld / %s", k, v, c); \
    fits_write_record(ofptr, record, &status);               \
}while(0)
#define WRITEDREC(k, v, c)                                  \
do{ int status = 0;                                         \
    snprintf(record, FLEN_CARD, "%-8s=%21g / %s", k, v, c); \
    fits_write_record(ofptr, record, &status);              \
}while(0)


/**
 * Try to store given FITS file adding boltwood's header
 * @param name (i) - full path to FITS file
 * @param data (i) - boltwood's header data
 * file stored in current directory under YYYY/MM/DD/hh:mm:ss.fits.gz
 * @return file descriptor for new notifications
 */
int store_fits(char *name, datarecord *data){
    DBG("Try to store %s", name);
    double t0 = dtime();
    long modtm = 0; // modification time - to change later
    char oldwd[PATH_MAX];
    fitsfile *fptr, *ofptr;
    int nkeys, bitpix, naxis;
    long naxes[2];
    uint16_t *imdat = NULL;
    TRYFITS(fits_open_file, &fptr, name, READONLY);
    TRYFITS(fits_get_img_param, fptr, 2, &bitpix, &naxis, naxes);
    TRYFITS(fits_get_hdrspace, fptr, &nkeys, NULL);
    DBG("%d keys", nkeys);
    green("open & get hdr: %gs\n", dtime() - t0);
    char dateobs[FLEN_KEYWORD], timeobs[FLEN_KEYWORD];
    TRYFITS(fits_read_key, fptr, TSTRING, "DATE-OBS", dateobs, NULL);
    DBG("DATE-OBS: %s", dateobs);
    TRYFITS(fits_read_key, fptr, TSTRING, "START", timeobs, NULL);
    DBG("START: %s", timeobs);
    TRYFITS(fits_read_key, fptr, TLONG, "UNIXTIME", &modtm, NULL);
    DBG("MODTM: %ld", modtm);
    if(!getcwd(oldwd, PATH_MAX)) ERR("getcwd()");
    DBG("Current working directory: %s", oldwd);
    green("+ got headers: %gs\n", dtime() - t0);
    gotodir(dateobs);
    char newname[PATH_MAX];
    snprintf(newname, PATH_MAX, "%s.fits.gz", timeobs);
    // remove if exists
    unlink(newname);
    TRYFITS(fits_create_file, &ofptr, newname);
    TRYFITS(fits_create_img, ofptr, bitpix, naxis, naxes);
    // copy all keywords
    int i, status;
    char record[81];
    // copy all the user keywords (not the structural keywords)
    DBG("got %d user keys, copy them", nkeys);
    for(i = 1; i <= nkeys; ++i){
        status = 0;
        fits_read_record(fptr, i, record, &status);
        DBG("status: %d", status);
        if(!status && fits_get_keyclass(record) > TYP_CMPRS_KEY)
            fits_write_record(ofptr, record, &status);
        else
            fits_report_error(stderr, status);
    }
    DBG("Boltwood data");
    // now the file copied -> add header from Boltwood's sensor
    while(data->varname){
        if(data->type == INTEGR){
            WRITEIREC(data->keyname, data->data.i, data->comment);
        }else{
           WRITEDREC(data->keyname, data->data.d, data->comment);
        }
        ++data;
    }
    long npix = naxes[0]*naxes[1];
    imdat = MALLOC(uint16_t, npix);
    int anynul = 0;
    TRYFITS(fits_read_img, fptr, TUSHORT, 1, npix, NULL, imdat, &anynul);
    DBG("read, anynul = %d", anynul);
    TRYFITS(fits_write_img, ofptr, TUSHORT, 1, npix, imdat);
ret:
    FREE(imdat);
    status = 0;
    if(fptr)  fits_close_file(fptr, &status);
    status = 0;
    if(ofptr){
        fits_close_file(ofptr, &status);
        modifytimestamp(newname, (time_t)modtm);
    }
    // as cfitsio removes old file instead of trunkate it, we need to refresh inotify every time!
    int fd = watch_fits(name);
    if(chdir(oldwd)){ // return back to BD root directory
        ERR("Can't chdir");
    };
   return fd;
}
