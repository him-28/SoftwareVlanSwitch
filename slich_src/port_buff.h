//
//  port_buff.h
//  Switch
//
//  Created by Michal Duczynski on 21.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef test1_port_buff_h
#define test1_port_buff_h

#include "basic_libs.h"
#include <pthread.h>
#include "fifo.h"
#include "ethernet_frame.h"

#define PORT_BUFFER_SIZE 6000

typedef struct port_buff_s{
    fifo_t* buff;
    uint32_t current_size;
} port_buff;

port_buff* port_buff_create(){
    port_buff* b = calloc(1,sizeof(port_buff));
    b->buff = fifo_new();
    return b;
}

void port_buff_destroy(port_buff* b){
    fifo_free(b->buff, (void(*)(void*))eth_frame_free);
    free(b);
}

bool port_buff_empty(port_buff* b){ return (fifo_empty(b->buff)==1);}

bool port_buff_push(port_buff* b, eth_frame* eth){
    if(b->current_size + eth->size > PORT_BUFFER_SIZE) return false;
    b->current_size += eth->size;
    
    fifo_add(b->buff, eth);
    
    return true;
}

eth_frame* port_buff_pop(port_buff* b){
    eth_frame* eth = (eth_frame*) fifo_remove(b->buff);
    b->current_size -= eth->size;
    return eth;
}


#endif
