//
//  port_conf.h
//  switch
//
//  Created by Duczi on 24.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef switch_port_conf_h
#define switch_port_conf_h

#include "basic_libs.h"

typedef struct vlan_tag_s {
    uint16_t num;
    bool tagged;
} vlan_tag;

typedef struct port_conf_s {
    uint16_t listening_port;
    struct sockaddr_in recv_addr;
    vlan_tag* tags;
    int num_of_tags;
    uint16_t default_vlan;
}port_conf;


void port_conf_destroy(port_conf* p ){
    free(p->tags);
}

#define PORT_CONF_VLAN_MAX 4095
#define PORT_CONF_REP_LEN 200
/*buffer length should be port_conf_REP_LEN */
void port_conf_print(char* buffer, port_conf* p){
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(p->recv_addr.sin_addr), str, INET_ADDRSTRLEN);
    
    char tags [PORT_CONF_REP_LEN];
    char * tags_ch = (char*)tags;
    for (int i=0; i<p->num_of_tags; i++) {
        tags_ch += sprintf(tags_ch, "%hu", p->tags[i].num);
        if (p->tags[i].tagged) {
            tags_ch += sprintf(tags_ch, "t");
        }
        tags_ch += sprintf(tags_ch, ",");
    }
    //remove last ','
    *(tags_ch-1)='\0';
    
    sprintf(buffer, "%hu/%s:%u/%s\n",p->listening_port, str, (unsigned)ntohs(p->recv_addr.sin_port), tags);
}

/* port_conf is allocated by the procedure, you should release the memory */
port_conf* port_conf_from_str(char * str){
    
    port_conf* conf = calloc(1,sizeof(port_conf));
    sscanf(str,"%hu/",&conf->listening_port);
    str = strchr(str, '/'); str++;
    
    if (*str != '/') {
        char recv_addr_str [NI_MAXHOST];
        uint16_t recv_port;
        sscanf(str, "%30[^:]:%hu/", recv_addr_str, &recv_port);
        str = strchr(str, '/'); str++;
        
        struct addrinfo addr_hints;
        struct addrinfo *addr_result;
        
        (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_DGRAM;
        addr_hints.ai_protocol = IPPROTO_UDP;
        if (getaddrinfo(recv_addr_str, NULL, &addr_hints, &addr_result) != 0) {
            free(conf);
            perror("getaddrinfo");
            return NULL;
        }
        
        conf->recv_addr.sin_family = AF_INET; // IPv4
        conf->recv_addr.sin_addr.s_addr =
        ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
        conf->recv_addr.sin_port = htons(recv_port); // port from the command line
        
        freeaddrinfo(addr_result);
    }else{
        str++;
    }
    
    
    //parse vlans
    if(strlen(str)>0){
        char vlans [PORT_CONF_REP_LEN];
        strcpy(vlans, str);
        size_t size = strlen(vlans);
        vlans[size] = ',';
        vlans[size+1]='\0';
        int i=0;
        int commas = 0;
        while (vlans[i]!='\0') {
            if (vlans[i++]==',')
                commas++;
        }
        conf->num_of_tags = commas;
        if(commas<=0) return conf;
        conf->tags = (vlan_tag*) calloc(commas, sizeof(vlan_tag));
        
        
        i=0;
        char * tags_ch = vlans;
        while (*tags_ch != '\0' ) {
            sscanf(tags_ch, "%hu", &conf->tags[i].num);
            tags_ch = strchr(tags_ch, ',');
            if (*(tags_ch-1)=='t') {
                conf->tags[i].tagged = true;
            }else{
                conf->default_vlan = conf->tags[i].num;
            }
            tags_ch++; //ignore the comma
            i++;
        }
        
    }

    return conf;
}

bool port_conf_in_vlan(port_conf* p, uint16_t vlan){
    for (int i=0; i < p->num_of_tags; i++) {
        if(p->tags[i].num == vlan) return true;
    }
    return false;
}

#endif
