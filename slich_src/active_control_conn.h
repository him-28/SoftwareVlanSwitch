//
//  active_control_conn.h
//  switch
//
//  Created by Duczi on 23.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#ifndef switch_active_control_conn_h
#define switch_active_control_conn_h

#include "basic_libs.h"
#include "port_conf.h"
#include <unistd.h>

/* NOT THREAD SAFE !!!! */

static int active_control_conn_fd = -1;

bool active_control_conn_write(const char * c){
    
    printf("%s",c);
    //!!!!!!!!!!!!!!!!! UNCOMMENT !!!!!!!
    //if (active_control_conn_fd<0) syserr("active_control_conn_fd negative");
    if (active_control_conn_fd<0) return false;
    ssize_t snd_len;
    if (strlen(c)>=PORT_CONF_REP_LEN) return false;
    static char buffer[PORT_CONF_REP_LEN];
    strncpy(buffer, c, PORT_CONF_REP_LEN);
    snd_len = write(active_control_conn_fd, buffer, PORT_CONF_REP_LEN);
    if (snd_len != PORT_CONF_REP_LEN){
        perror("writing to client socket");
        return false;
    }
    return true;
}

#endif
