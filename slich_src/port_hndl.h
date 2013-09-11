//
//  port_hndl.h
//  Switch
//
//  Created by Michal Duczynski on 21.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef test1_port_hndl_h
#define test1_port_hndl_h

#include "basic_libs.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "port_buff.h"
#include "port_conf.h"
#include "ethernet_frame.h"
#include "mac_addr_table.h"


static pthread_attr_t _port_hndl_th_attr;
static pthread_rwlock_t _port_hnlds_lock;
static void port_hndls_init(){
    pthread_attr_init(&_port_hndl_th_attr);
    pthread_attr_setdetachstate(&_port_hndl_th_attr, PTHREAD_CREATE_JOINABLE);
    
    pthread_rwlock_init(&_port_hnlds_lock, NULL);
    
    mac_addr_table_init();
}

struct counter_s{
    uint32_t sent;
    uint32_t recv;
    uint32_t errs;
};

typedef struct port_hndl_s {
    port_conf* conf;
    
    port_buff* buff;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_empty_cv;
    
    struct counter_s counter;
    
    pthread_t writer;
    pthread_t reader;
    
    int udp_sock;
    
    bool* ending_writer;
    bool* ending_reader;
    
}port_hndl;



void* reader_th(void* s_ptr);
void* writer_th(void* s_ptr);


//the port_hndls array is kept always sorted
int port_hndl_compare (const void * a, const void * b);

#define PORT_HNDLS_MAX 10
static port_hndl* port_hndls [PORT_HNDLS_MAX];

void port_hndls_sort(){
    qsort(port_hndls, PORT_HNDLS_MAX, sizeof(port_hndl*), port_hndl_compare);
}

void _port_hndl_destroy(port_hndl* p){
    *p->ending_reader = true;
    shutdown(p->udp_sock, SHUT_RDWR);
    close(p->udp_sock);
    pthread_join(p->reader, NULL);

    *p->ending_writer = true;
    pthread_mutex_lock(&p->buffer_mutex);
    pthread_cond_signal(&p->buffer_empty_cv);
    pthread_mutex_unlock(&p->buffer_mutex);
    
    pthread_join(p->writer, NULL);
    
    port_conf_destroy(p->conf);
    port_buff_destroy(p->buff);
    pthread_mutex_destroy(&p->buffer_mutex);
    pthread_cond_destroy(&p->buffer_empty_cv);
    
    free(p);
}

void port_hndl_destroy(port_hndl* p){
    pthread_rwlock_wrlock(&_port_hnlds_lock);
    for(int i=0; i<PORT_HNDLS_MAX;i++)
        if (port_hndls[i]==p) port_hndls[i]=NULL;
    
    _port_hndl_destroy(p);
    
    port_hndls_sort();
    
    pthread_rwlock_unlock(&_port_hnlds_lock);
}

void port_hndls_cleanup(){
    pthread_rwlock_wrlock(&_port_hnlds_lock);
    debug("CLEANUP... PLEASE WAIT");
    for(int i=0; i<PORT_HNDLS_MAX;i++){
        if(port_hndls[i]){
            _port_hndl_destroy(port_hndls[i]);
        }
    }
    pthread_rwlock_unlock(&_port_hnlds_lock);
    
    pthread_attr_destroy(&_port_hndl_th_attr);
    pthread_rwlock_destroy(&_port_hnlds_lock);
    
    mac_addr_table_destroy();
}

port_hndl* port_hndls_get(uint16_t listening_port){
    pthread_rwlock_rdlock(&_port_hnlds_lock);
    int i;
    for(i=0; i<PORT_HNDLS_MAX;i++){
        if (port_hndls[i] &&
            ( listening_port == port_hndls[i]->conf->listening_port)) {
            break;
        }
    }
    port_hndl* p;
    if (i==PORT_HNDLS_MAX) {
        p = NULL;
    }else{
        p = port_hndls[i];
    }
    pthread_rwlock_unlock(&_port_hnlds_lock);
    return p;
}

port_hndl* port_hndl_create(port_conf* conf){
    pthread_rwlock_wrlock(&_port_hnlds_lock);
    int i;

    for(i=0; i<PORT_HNDLS_MAX;i++)
        if (!port_hndls[i]) break;
    
    port_hndl* p;
    if (i<PORT_HNDLS_MAX) {
        p = calloc(1,sizeof(port_hndl));
        
        p->buff = port_buff_create();
        p->conf = conf;
        p->ending_reader = calloc(1,sizeof(bool));
        p->ending_writer = calloc(1,sizeof(bool));
        *p->ending_reader = false;
        *p->ending_writer = false;
        p->counter.errs=0;
        p->counter.sent=0;
        p->counter.recv=0;
        
        port_hndls[i]=p;
        port_hndls_sort();
        
        //setup the udp socket==================
        p->udp_sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
        if (p->udp_sock < 0) {
            perror("socket");
            pthread_rwlock_unlock(&_port_hnlds_lock);
            return NULL;
        }
        
        struct sockaddr_in port_addr;
        port_addr.sin_family = AF_INET; // IPv4
        port_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
        port_addr.sin_port = htons(p->conf->listening_port);
        // bind the socket to a concrete address
        if (bind(p->udp_sock, (struct sockaddr *) &port_addr,
                 (socklen_t) sizeof(port_addr)) < 0){
            perror("bind");
            pthread_rwlock_unlock(&_port_hnlds_lock);
            return NULL;
        }
        //======================================
        
        pthread_mutex_init(&p->buffer_mutex, NULL);
        pthread_cond_init (&p->buffer_empty_cv, NULL);
        
        int rc = pthread_create(&p->writer, &_port_hndl_th_attr, writer_th, p);
        if (rc==-1) syserr("pthread_create");
        
        rc = pthread_create(&p->reader, &_port_hndl_th_attr, reader_th, p);
        if (rc==-1) syserr("pthread_create");
        
    }else p = NULL;
    pthread_rwlock_unlock(&_port_hnlds_lock);
    return p;
}

void* reader_th(void* s_ptr){
    port_hndl* p = (port_hndl*) s_ptr;
    
    debug("reader for %hu",p->conf->listening_port);
    
    //local copies
    uint16_t listening_port = p->conf->listening_port;
    bool* ending = p->ending_reader;
    
    struct sockaddr_in client_addr;
    ssize_t resplen;
    char buffer[ETHERNET_FRAME_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);
    
    do {
        resplen = recvfrom(p->udp_sock, buffer, sizeof(buffer), 0,
                           (struct sockaddr *) &client_addr, &client_addr_len);

        if (resplen < 0 && !(*ending)){
            perror("error on datagram from client socket");
        } else if(resplen>0){
            char sender[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), sender, INET_ADDRSTRLEN);
            debug("sender %s:%u",sender,(unsigned)ntohs(client_addr.sin_port));
            
            char recv_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(p->conf->recv_addr.sin_addr), recv_str, INET_ADDRSTRLEN);
            debug("assigned recv %s:%u",recv_str,(unsigned)ntohs(p->conf->recv_addr.sin_port));
            
            debug("reader %hu: from socket: %zd bytes", listening_port, resplen);
            
            pthread_mutex_lock(&p->buffer_mutex);
            p->counter.recv++;
            pthread_mutex_unlock(&p->buffer_mutex);
            
            eth_frame* eth = eth_frame_new(buffer, (uint32_t) resplen);
            
            bool all_ok = true;
            
            //Basic check: eth format
            if (!eth) {
                debug("data doesn't match eth frame format");
                pthread_mutex_lock(&p->buffer_mutex);
                p->counter.errs++;
                pthread_mutex_unlock(&p->buffer_mutex);
                all_ok = false;
            }else eth_frame_print(eth);
            //Check vlan
            uint16_t vlan;
            uint16_t default_vlan = p->conf->default_vlan;
            if (all_ok) {
                debug("%hu: default vlan %hu",listening_port, default_vlan);
                
                vlan = eth_frame_vlan(eth);
                debug("%hu: vlan %hu", listening_port, vlan);
                if (vlan==0) {
                    if (default_vlan == 0) {
                        debug("%hu: dropping : untagged frame and no default vlan",listening_port);
                        all_ok = false;
                    } else {
                        eth_frame_add_vlan_tag(eth, default_vlan);
                        vlan = default_vlan;
                    }
                } else if ( !port_conf_in_vlan(p->conf, vlan)){
                    debug("%hu: dropping : vlan not in vlans", listening_port);
                    all_ok = false;
                }
            }
            if (all_ok && recv_str[0]=='0' ) {
                debug("%hu: no recv yet, assigning", listening_port);
                p->conf->recv_addr = client_addr;
                eth_mac mac_src = eth_frame_source_mac(eth);
                //mac_addr_table is thread safe
                mac_addr_table_add(mac_src, vlan, p->conf->listening_port );
                strncpy(recv_str, sender, INET_ADDRSTRLEN);
            }
            //Check if sender matches the assigned receiver
            if (all_ok && !(strcmp(sender, recv_str)==0 &&
                client_addr.sin_port == p->conf->recv_addr.sin_port)) {
                debug("%hu: dropping : recv doesn't match", listening_port);
                all_ok = false;
            }
            if (all_ok) {
                eth_mac mac_dest = eth_frame_dest_mac(eth);
                uint16_t dest_port = mac_addr_table_get(mac_dest, vlan);
                
                port_hndl* dest_port_hndl = NULL;
                if (dest_port != 0) {
                    dest_port_hndl = port_hndls_get(dest_port);
                }
                
                if (eth_mac_is_broadcast(&mac_dest) ||
                    eth_mac_is_multicast(&mac_dest) ||
                    dest_port == 0 || //we didn't find a port in mac_addr
                    !dest_port_hndl || //we found but the port was deleted
                    !port_conf_in_vlan(dest_port_hndl->conf, vlan)) {
                    debug("%hu : broadcast",listening_port);
                    for (int i=0; i<PORT_HNDLS_MAX && port_hndls[i]; i++) {
                        port_hndl* dest_port_hndl = port_hndls[i];
                        if (dest_port_hndl != p &&
                            port_conf_in_vlan(dest_port_hndl->conf, vlan))
                        {
                            eth_frame* eth_cpy = eth_frame_cpy(eth);
                            pthread_mutex_lock(&dest_port_hndl->buffer_mutex);
                            port_buff_push(dest_port_hndl->buff, eth_cpy);
                            
                            pthread_cond_signal(&dest_port_hndl->buffer_empty_cv);
                            pthread_mutex_unlock(&dest_port_hndl->buffer_mutex);
                        }
                    }
                    eth_frame_free(eth);
                }else{
                    if (dest_port_hndl != p){
                        pthread_mutex_lock(&dest_port_hndl->buffer_mutex);
                        port_buff_push(dest_port_hndl->buff, eth);
                        
                        pthread_cond_signal(&dest_port_hndl->buffer_empty_cv);
                        pthread_mutex_unlock(&dest_port_hndl->buffer_mutex);
                        
                    }else debug ("%hu : no forward back to src port", listening_port);
                }
            }
        }
    } while (!(*ending) && resplen > 0);
    
    debug("%hu : ending reader",listening_port);
    
    free(ending);

    return 0;
}

void* writer_th(void* s_ptr){
    port_hndl* p = (port_hndl*) s_ptr;
    
    debug("%hu, writer",p->conf->listening_port);
    
    eth_frame* eth;
    while (1) {
        pthread_mutex_lock(&p->buffer_mutex);
        while (port_buff_empty(p->buff) && !(*p->ending_writer)) {
        	debug("waiting");
            pthread_cond_wait(&p->buffer_empty_cv, &p->buffer_mutex);
            debug("received signal");
        }
        
        if(*p->ending_writer==true) {debug("breaking"); break;}
        
        eth = port_buff_pop(p->buff);
        
        debug("%hu: writer",p->conf->listening_port);
        eth_frame_print(eth);
        
        if(p->conf->recv_addr.sin_port==0) {
            debug("%hu: dropping : no recv addr",p->conf->listening_port);
            p->counter.errs++;
        } else {
            uint16_t vlan = eth_frame_vlan(eth);
            if (!port_conf_in_vlan(p->conf, vlan)) {
                debug("don't know what's happening");
            }else if(vlan == p->conf->default_vlan){
                eth_frame_remove_vlan_tag(eth);
                debug("rmv tag");
                eth_frame_print(eth);
            }
            
            ssize_t snd_len = sendto(p->udp_sock, eth->data, eth->size, 0,
                                     (struct sockaddr *) &p->conf->recv_addr,
                                     (socklen_t) sizeof(p->conf->recv_addr));
            if (snd_len != eth->size){
                perror("error on sending datagram to client socket");
                p->counter.errs++;
            }else p->counter.sent++;
        }
        eth_frame_free(eth);
        pthread_mutex_unlock(&p->buffer_mutex);
    }
    
    debug("%hu : ending writer",p->conf->listening_port);
    
    return 0;
}

int port_hndl_compare (const void * a, const void * b)
{
    port_hndl* p1 = *(port_hndl**) a;
    port_hndl* p2 = *(port_hndl**) b;
    
    if (!p1 && p2) return 1;
    else if (p1 && !p2) return -1;
    else if (!p1 && !p2) return 0;
    else{
        if ( p1->conf->listening_port <  p2->conf->listening_port ) return -1;
        else if ( p1->conf->listening_port == p2->conf->listening_port ) return 0;
        else return 1;
    }
    
}

#endif
