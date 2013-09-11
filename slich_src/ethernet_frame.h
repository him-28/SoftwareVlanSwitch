//
//  ethernet_frame.h
//  switch
//
//  Created by Duczi on 25.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef switch_ethernet_frame_h
#define switch_ethernet_frame_h

#include "basic_libs.h"
#define ETHERNET_FRAME_SIZE 1530

typedef struct eth_frame_s{
    char data[ETHERNET_FRAME_SIZE];
    uint32_t size;
} eth_frame;

typedef struct{
    char data[6];
}eth_mac;

bool eth_mac_eq(eth_mac* m1, eth_mac* m2)
{return (memcmp(m1->data, m2->data, 6)==0);}

eth_frame* eth_frame_new(char* str, uint32_t slen){
    if (slen > ETHERNET_FRAME_SIZE || slen < 16) return NULL;
    eth_frame* eth = calloc(1,sizeof(eth_frame));
    memcpy(eth->data, str, slen);
    eth->size = slen;
    return eth;
}

eth_frame* eth_frame_cpy(eth_frame* e){
    eth_frame* eth = calloc(1,sizeof(eth_frame));
    memcpy(eth->data, e->data, e->size);
    eth->size = e->size;
    return eth;
}

void hexdump(void *ptr, int buflen) {
  unsigned char *buf = (unsigned char*)ptr;
  int i, j;
  for (i=0; i<buflen; i+=16) {
    printf("%06x: ", i);
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        printf("%02x ", buf[i+j]);
      else
        printf("   ");
    printf(" ");
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
    printf("\n");
  }
}

void eth_frame_print(eth_frame* eth){
	hexdump(&eth->data[0], eth->size);
}

void eth_frame_free(eth_frame* eth){
    free(eth);
}

uint16_t eth_frame_vlan(eth_frame* eth){
    uint16_t* tagged = (uint16_t*) &eth->data[12];
    if (ntohs(*tagged) != 0x8100) return 0;
    else{
        uint16_t vlan = *((uint16_t* ) &eth->data[14]);
        vlan = ntohs(vlan) & 0xfff;
        return vlan;
    }
}

void eth_frame_add_vlan_tag(eth_frame* eth, uint16_t defualt_vlan){
    
    memmove(eth->data+12+4, eth->data+12, eth->size-12);
    eth->size += 4;
    memset (eth->data+12, '\0', 4);
    //adding vlan tagged field
    uint16_t tagged_uint = htons(0x8100);
    char* tagged = (char*) &tagged_uint;
    memcpy(eth->data + 12, tagged, 2);
    
    //adding vlan tag
    defualt_vlan = htons(defualt_vlan);
    char* vlan_tag = (char*)&defualt_vlan;
    memcpy(eth->data+14, vlan_tag, 2);
    
}

void eth_frame_remove_vlan_tag(eth_frame* eth){
    memmove(eth->data+12, eth->data+12+4, eth->size-16);
    eth->size -= 4;
}


eth_mac eth_frame_dest_mac(eth_frame* eth){
    eth_mac mac;
    memcpy(mac.data, &eth->data[0], 6);
    return mac;
}

eth_mac eth_frame_source_mac(eth_frame* eth){
    eth_mac mac;
    memcpy(mac.data, &eth->data[6], 6);
    return mac;
}

bool eth_mac_is_broadcast(eth_mac* mac){
    return (mac->data[0] & 1);
}

bool eth_mac_is_multicast(eth_mac* mac){
    return eth_mac_is_broadcast(mac);
}



#endif
