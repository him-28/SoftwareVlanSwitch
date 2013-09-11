//
//  cmd_buff.h
//  switch
//
//  Created by Duczi on 23.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef switch_cmd_buff_h
#define switch_cmd_buff_h

#include "basic_libs.h"

static void (*cmd_buff_parse)(char*);

typedef struct cmd_buff_s {
    char buff[100];//command length
    size_t offset;
}cmd_buff;

void cmd_buff_init(cmd_buff* b){
    memset(b->buff, '\0', 100);
    b->offset = 0;
}

/* returns char* to a place where a single char can be written
 * returns NULL when the buffer is full */
char* cmd_buff_ptr(cmd_buff* b){
    if (b->offset == 100) {
        cmd_buff_init(b);
        return NULL;
    }
    return &b->buff[b->offset];
}
//REMEMBER TO CHANGE TO NEWLINE
void cmd_buff_advance(cmd_buff* b){
    //the last char we read was the newline
    if (b->buff[b->offset] == '\n') {
        b->buff[b->offset] = '\0';
        cmd_buff_parse(b->buff);
        cmd_buff_init(b);
    }else{
        b->offset++;
    }
}


#endif
