//
//  basic_libs.h
//  switch
//
//  Created by Duczi on 23.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef switch_basic_libs_h
#define switch_basic_libs_h

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

const static int _debug_flag = 1;

void debug(const char *fmt, ...)
{
    if(_debug_flag ==1){
    va_list fmt_args;
    
    fprintf(stderr, "D: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end (fmt_args);
    fprintf(stderr, "\n");
    }
}

void syserr(const char *fmt, ...)
{
    va_list fmt_args;
    
    fprintf(stderr, "ERROR: ");
    
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end (fmt_args);
    perror(" ");
    exit(EXIT_FAILURE);
}

/* Author note:
 * though it may seem strange calloc(1,sizeof(bleble))
 * the fact that c doesn't clear the memory on malloc infuriates me
 * so the calloc is the remedy
 */

#endif
