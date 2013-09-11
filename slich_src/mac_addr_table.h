//
//  mac_addr_table.h
//  Switch
//
//  Created by Michal Duczynski on 21.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef test1_mac_addr_table_h
#define test1_mac_addr_table_h

#include "basic_libs.h"
#include "ethernet_frame.h"
#include <pthread.h>

typedef struct mac_to_port_s {
    eth_mac mac;
    uint16_t vlan;
    uint16_t port;
    
    uint16_t time;
} mac_to_port;

//4096
#define MAC_ADDR_TABLE_MAX 30

//============= THREAD SAFE :)

static mac_to_port _mac_addr_tbl[MAC_ADDR_TABLE_MAX];
static uint16_t _mac_addr_counter = 0;
static uint16_t _mac_addr_next_time = 1;
static pthread_rwlock_t _mac_addr_lock;

static void mac_addr_table_init(){
    memset(&_mac_addr_tbl, 0, sizeof(mac_to_port)*MAC_ADDR_TABLE_MAX);
    pthread_rwlock_init(&_mac_addr_lock, NULL);
}

static void mac_addr_table_destroy(){
    pthread_rwlock_destroy(&_mac_addr_lock);
}

uint16_t mac_addr_table_get(eth_mac mac, uint16_t vlan){
    pthread_rwlock_rdlock(&_mac_addr_lock);
    for (int i=0; i<MAC_ADDR_TABLE_MAX; i++)
        if ( eth_mac_eq(&_mac_addr_tbl[i].mac, &mac) &&
            _mac_addr_tbl[i].vlan == vlan){
            pthread_rwlock_unlock(&_mac_addr_lock);
            return _mac_addr_tbl[i].port;
        }
    pthread_rwlock_unlock(&_mac_addr_lock);
    return 0;
}

void mac_addr_table_add(eth_mac mac, uint16_t vlan, uint16_t port){

    pthread_rwlock_wrlock(&_mac_addr_lock);
    
    int pos= -1;

    //search for pair (mac,vlan)
    for (int i=0; i<MAC_ADDR_TABLE_MAX; i++)
        if (eth_mac_eq(&_mac_addr_tbl[i].mac, &mac) &&
            _mac_addr_tbl[i].vlan == vlan){
            pos = i; break;
        }
    
    if (pos!=-1) {
        _mac_addr_tbl[pos].time = _mac_addr_next_time++;
        _mac_addr_tbl[pos].port = port;
        pthread_rwlock_unlock(&_mac_addr_lock);
        return;
    }
    if (_mac_addr_counter >= MAC_ADDR_TABLE_MAX) {
        //replace the oldest one ie with smallest time
        uint16_t smallest_time = MAC_ADDR_TABLE_MAX*2;
        
        for (int i=0; i<MAC_ADDR_TABLE_MAX; i++) {
            if (_mac_addr_tbl[i].time < smallest_time) {
                smallest_time = _mac_addr_tbl[i].time;
                pos = i;
            }
        }
        //substract MAC_ADDR_TABLE_MAX from all times
        if (smallest_time == MAC_ADDR_TABLE_MAX+1) {
            _mac_addr_next_time -= MAC_ADDR_TABLE_MAX+1;
            for (int i=0; i<MAC_ADDR_TABLE_MAX; i++)
                _mac_addr_tbl[i].time -= MAC_ADDR_TABLE_MAX+1;
        }
    }else{
        for (int i=0; i<MAC_ADDR_TABLE_MAX; i++)
            if (_mac_addr_tbl[i].time ==0){
                pos = i;
                break;
            }
        _mac_addr_counter++;
    }
    
    
    _mac_addr_tbl[pos].mac = mac;
    _mac_addr_tbl[pos].port=port;
    _mac_addr_tbl[pos].vlan=vlan;
    _mac_addr_tbl[pos].time=_mac_addr_next_time++;
    
    debug("adding at pos %d", pos);
    
    pthread_rwlock_unlock(&_mac_addr_lock);
}

#endif
