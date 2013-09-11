//
//  main.c
//  switch
//
//  Created by Michal Duczynski on 20.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#include "basic_libs.h"

#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "port_hndl.h"
#include "cmd_buff.h"
#include "active_control_conn.h"

#define MAX_ACTIVE_PORTS_LIMIT 100
#define CONTROL_CONN_MAX 20

static int control_port_num = 42420;

#define TRUE 1
#define FALSE 0

static int finish = FALSE;

/* intrerrupt signal handler */
static void catch_int (int sig) {
    finish = TRUE;
    debug("Signal %d catched. No new connections will be accepted.\n", sig);
}

void set_config(port_conf* p){
    port_hndl* current = port_hndls_get(p->listening_port);
    if (!current) {
        if (p->num_of_tags <= 0) {
            active_control_conn_write("ERR can't remove nonexistent port conf \n");
            port_conf_destroy(p);
            return;
        }
        if( port_hndl_create(p) ){
            active_control_conn_write("OK\n");
        }else{
            active_control_conn_write("ERR too many active ports \n");
            port_conf_destroy(p);
        }
    } else {
        //edit current
        if (p->num_of_tags <= 0) {
            port_hndl_destroy(current);
            debug("remove port conf");
            active_control_conn_write("OK\n");
        }else{
            //can't change only a part of port conf
            active_control_conn_write("can't change only a part of port conf\n");
            port_conf_destroy(p);
        }
    }
}

void get_config(){
    debug( "Get config");
    char buffer [PORT_CONF_REP_LEN];
    for (int i=0; i<PORT_HNDLS_MAX && port_hndls[i]; i++) {
        port_conf_print(buffer, port_hndls[i]->conf);
        active_control_conn_write(buffer);
    }
    active_control_conn_write("END\n");
}

void counters(){
    debug( "Counters\n");
    char buffer [PORT_CONF_REP_LEN];
    for (int i=0; i<PORT_HNDLS_MAX && port_hndls[i]; i++) {
        sprintf(buffer, "%hu rcvd:%d sent:%d errs:%d\n",
                port_hndls[i]->conf->listening_port,
                port_hndls[i]->counter.recv,
                port_hndls[i]->counter.sent,
                port_hndls[i]->counter.errs);
        active_control_conn_write(buffer);
    }
    active_control_conn_write("END\n");
}

void cleanup(){
    port_hndls_cleanup();
    /*==============================================*/
    /*==============================================*/
    /*==============================================*/
}



void parse_args(int argc, char ** argv){
    int c;
    while ((c = getopt (argc, argv, "c:p:")) != -1)
        switch (c)
    {
        case 'c':
            //set the TCP port number for control interface
             printf("-c");
            control_port_num = atoi(optarg);
            break;
        case 'p':{
            printf("-p");
            port_conf* p = port_conf_from_str(optarg);
            if (p) {
                set_config(p);
            }else{
                debug( "invalid port config: %s\n", optarg);
            }
            break;
        }
        case '?':
            if (optopt == 'c')
                debug( "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                debug( "Unknown option `-%c'.\n", optopt);
            else
                debug( "Unknown option character `\\x%x'.\n",optopt);
            exit(EXIT_FAILURE);
        default:
            abort ();
    }
}

void cmd_parse(char* str){
    char buf [PORT_CONF_REP_LEN];
    debug("->%s\n",str);
    if (sscanf(str, "setconfig %s",buf)) {
        port_conf* p =port_conf_from_str(buf);
        if (p) set_config(p);
        else active_control_conn_write("ERR ivalid port config");
    }else if(strncmp(str, "getconfig",9)==0){
        get_config();
    }else if(strncmp(str, "counters",8)==0){
        counters();
    }else{
        active_control_conn_write("ERR no such command\n");
    }
}


void control_thread(){
    struct pollfd client[CONTROL_CONN_MAX];
    struct sockaddr_in server;
    cmd_buff cmd_buffs[CONTROL_CONN_MAX];
    ssize_t rval;
    int msgsock, activeClients, i, ret;
    
    cmd_buff_parse = &cmd_parse;
    for (i=0; i<CONTROL_CONN_MAX; i++) {
        cmd_buff_init(&cmd_buffs[i]);
    }
    
    /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
    for (i = 0; i < CONTROL_CONN_MAX; ++i) {
        client[i].fd = -1;
        client[i].events = POLLIN;
        client[i].revents = 0;
    }
    activeClients = 0;
    
    /* Tworzymy gniazdko centrali */
    client[0].fd = socket(PF_INET, SOCK_STREAM, 0);
    if (client[0].fd < 0) {
        perror("Opening stream socket");
        exit(EXIT_FAILURE);
    }
    
    server.sin_family = AF_INET;
    
    // listening on all interfaces
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // listening on port: control_port_num
    server.sin_port = htons(control_port_num);
    
    // bind the socket to a concrete address
    if (bind(client[0].fd, (struct sockaddr*)&server,
             (socklen_t)sizeof(server)) < 0) {
        perror("Binding stream socket");
        exit(EXIT_FAILURE);
    }
    
    printf("Control connection at port %d\n",control_port_num);
    
    // switch to listening (passive open)
    if (listen(client[0].fd, 5) == -1) {
        perror("Starting to listen");
        exit(EXIT_FAILURE);
    }
    
    /* Do pracy */
    do {
        for (i = 0; i < CONTROL_CONN_MAX; ++i)
            client[i].revents = 0;
        if (finish == TRUE && client[0].fd >= 0) {
            if (close(client[0].fd) < 0)
                perror("close");
            client[0].fd = -1;
        }
        
        /* Wait for 5000 ms */
        ret = poll(client, CONTROL_CONN_MAX, 5000);
        if (ret < 0)
            perror("poll");
        else if (ret > 0) {
            if (finish == FALSE && (client[0].revents & POLLIN)) {
                msgsock =
                accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
                if (msgsock == -1)
                    perror("accept");
                else {
                    for (i = 1; i < CONTROL_CONN_MAX; ++i) {
                        if (client[i].fd == -1) {
                            client[i].fd = msgsock;
                            activeClients += 1;
                            break;
                        }
                    }
                    if (i >= CONTROL_CONN_MAX) {
                        debug( "Too many control connections\n");
                        if (close(msgsock) < 0)
                            perror("close");
                    }
                }
            }
            for (i = 1; i < CONTROL_CONN_MAX; ++i) {
                if (client[i].fd != -1
                    && (client[i].revents & (POLLIN | POLLERR))) {
                    //read char by char
                    
                    active_control_conn_fd = client[i].fd;
                    
                    char* buf = cmd_buff_ptr(&cmd_buffs[i]);
                    if(!buf) perror("the cmd is to long");
                    rval = read(client[i].fd, buf, 1);
                    if (rval < 0) {
                        perror("Reading stream message");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    }
                    else if (rval == 0) {
                        debug( "Ending connection\n");
                        if (close(client[i].fd) < 0)
                            perror("close");
                        client[i].fd = -1;
                        activeClients -= 1;
                    }
                    else{
                        cmd_buff_advance(&cmd_buffs[i]);
                    }
                        
                }
            }
        }
        /*else
            debug( "control: waiting");*/
    } while (finish == FALSE || activeClients > 0);
    
    if (client[0].fd >= 0)
        if (close(client[0].fd) < 0)
            perror("Closing main socket");
    
    cleanup();
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char ** argv)
{
    /*  Ctrl-C end-signal handler */
    if (signal(SIGINT, catch_int) == SIG_ERR) {
        perror("Unable to change signal handler\n");
        exit(EXIT_FAILURE);
    }
    
    port_hndls_init();
    
    parse_args(argc, argv);
    
    
    //control_thread();
    
    /*port_conf* p = port_conf_from_str("42421//1,2t");
    if (p) set_config(p);
    */
    /*p = port_conf_from_str("42422//1,2t");
    if (p) set_config(p);
    
    p = port_conf_from_str("42423//1,2t");
    if (p) set_config(p);
    */
    
    //get_config();
    
    control_thread();
    
    
    return 0;
}

