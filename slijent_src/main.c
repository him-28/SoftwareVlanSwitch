//
//  slijent.c
//  slijent
//
//  Created by Duczi on 25.05.2013.
//  Copyright (c) 2013 Student 320321. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

void get_addr(char* str, struct sockaddr_in * server){
    char recv_addr_str [NI_MAXHOST];
    uint16_t recv_port;
    sscanf(str, "%30[^:]:%hu", recv_addr_str, &recv_port);
    
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;
    
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(recv_addr_str, NULL, &addr_hints, &addr_result) != 0) {
        perror("getaddrinfo"); exit(1);
    }
    
    server->sin_family = AF_INET; // IPv4
    server->sin_addr.s_addr =
    ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
    server->sin_port = htons(recv_port); // port from the command line
    
    freeaddrinfo(addr_result);

}


int fd;
int udp_sock;
struct sockaddr_in switch_addr;

void* from_switch_th(void* _v){
    struct sockaddr_in srvr_address;
    socklen_t rcva_len = (socklen_t) sizeof(srvr_address);
    ssize_t rcv_len;
    char buf[1518];  /* w tym max. 18 bajtów nagłówka */
    ssize_t wbytes;
    
    while (1) {
        memset(buf, 0, sizeof(buf));
        int len = (size_t) sizeof(buf) - 1;
        
        rcv_len = recvfrom(udp_sock, buf, len, 0,
                           (struct sockaddr *) &srvr_address, &rcva_len);
        
        if (rcv_len < 0) {
            perror("read"); exit(1);
        }
        (void) printf("read from socket: %zd bytes", rcv_len);
        
        wbytes = write(fd, buf, rcv_len);
        if (wbytes < 0) {
            perror("write()");
            exit(1);
        }
    }
}

int main(int argc, char** argv)
{
	char if_name [IFNAMSIZ+1];
	memset(&if_name[0],'\0',IFNAMSIZ);
    strncpy(&if_name[0],"siktap",IFNAMSIZ);
    
    //=================== slich addr from cmd line=======
    int c;
    while ((c = getopt (argc, argv, "d:s:")) != -1)
        switch (c)
    {
        case 'd':
            memset(&if_name[0],'\0',IFNAMSIZ);
    		strncpy(&if_name[0],optarg,IFNAMSIZ);
            break;
        case 's':{
            printf("-s");
            get_addr(optarg, &switch_addr);
            char recv_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &switch_addr.sin_addr, recv_str, INET_ADDRSTRLEN);
            printf("%s \n", recv_str);
            break;
        }
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n",optopt);
            exit(EXIT_FAILURE);
        default:
            abort ();
    }
    //=====================================================
    
    struct ifreq ifr;
    int err;
    
    //fprintf(stderr,"interface name: %s\n", if_name);
    
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open(/dev/net/tun)");
        return 1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, &if_name[0], IFNAMSIZ);
   
    
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        return 1;
    }
    
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock<0) {perror("sock"); return 1;}
    
    
    pthread_t from_switch;
    int rc = pthread_create(&from_switch, NULL, from_switch_th, NULL);
    if (rc==-1) {perror("pthread_create"); return 1;}
    rc = pthread_detach(from_switch);
    if (rc == -1) {
        perror("pthread_detach");
        exit(EXIT_FAILURE);
    }
    
    for (;;) {
        char buf[1518];  /* w tym max. 18 bajtów nagłówka */
        ssize_t rbytes;
        
        rbytes = read(fd, buf, sizeof(buf));
        if (rbytes < 0) {
            perror("read()");
            return 1;
        }
        printf("Read frame (%d bytes)\n", (int)rbytes);
        
        ssize_t snd_len = sendto(udp_sock, buf, rbytes, 0,
                         (struct sockaddr *) &switch_addr, (socklen_t) sizeof(switch_addr));
        if (snd_len != rbytes) {
            perror("partial / failed write");
        }
    }
}

// vim: sw=2:ts=2:et
