/* util.c - util functions */

/* Copyright (C) 2007 Keith Rarick and Philotic Inc.

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "util.h"

char *progname; /* defined as extern in util.h */

void
v()
{
}

static void
vwarnx(const char *err, const char *fmt, va_list args)
{
    fprintf(stderr, "%s: ", progname);
    if (fmt) {
        vfprintf(stderr, fmt, args);
        if (err) fprintf(stderr, ": %s", strerror(errno));
    }
    fputc('\n', stderr);
}

void
warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vwarnx(strerror(errno), fmt, args);
    va_end(args);
}

void
warnx(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vwarnx(NULL, fmt, args);
    va_end(args);
}

usec
usec_from_timeval(struct timeval *tv)
{
    return ((usec) tv->tv_sec) * SECOND + tv->tv_usec;
}

void
timeval_from_usec(struct timeval *tv, usec t)
{
    tv->tv_sec = t / SECOND;
    tv->tv_usec = t % SECOND;
}

usec
now_usec(void)
{
    int r;
    struct timeval tv;

    r = gettimeofday(&tv, 0);
    if (r != 0) return warnx("gettimeofday"), -1; // can't happen

    return usec_from_timeval(&tv);
}

/**
* Create an array from a string separated by some special char.
* *  Divides the src string in pieces, each delimited by token
* *  and storing the total of pieces in total
* * @param src String to parse
* * @param token Character delimiter to search.
* * @param total An integer variable passed as reference, which stores the total of
* * itens of the array
* * @return The array, where each item is one separeted by token
* *
* * \code
* *
* *  char **pieces;
* *  char *name = "This,is,a,string,of,test";
* *  int total, i;
* *  pieces = explode(name, ',', &total);
* *  for (i = 0; i < total; i++)
* *       printf("Piece %d: %s\n", i, *(pieces+i));
* * \endcode
* **/

uint64_t *explode(char *string, char separator, int *arraySize){
    int start = 0, i, k = 1, count = 2;
    char **strarr;
    for (i = 0; string[i] != '\0'; i++){
        /* how many rows do we need for our array? */
        if (string[i] == separator){
            count++;
        }
    }
    arraySize[0]=count-1;
    uint64_t *ret = malloc(sizeof(uint64_t) * *arraySize);
    /* count is at least 2 to make room for the entire string
 *      * and the ending NULL */
    strarr = calloc(count, sizeof(char*));
    i = 0;
    while (*string++ != '\0'){
        if (*string == separator){
            strarr[i] = calloc(k - start + 2,sizeof(char));
            strncpy(strarr[i], string - k + start, k - start);
            strarr[i][k - start + 1] = '\0'; /* guarantee null termination */
            start = k;
            start++;
            i++;
        }
        k++;
    }
    /* copy the last part of the string after the last separator */
    strarr[i] = calloc(k - start,sizeof(char));
    strncpy(strarr[i], string - k + start, k - start - 1);
    strarr[i][k - start - 1] = '\0'; /* guarantee null termination */
    strarr[++i] = NULL;
    
    for(i = 0; i < *arraySize; i++)
    {
	    ret[i] = atoi(strarr[i]);
    }

    return ret;
}