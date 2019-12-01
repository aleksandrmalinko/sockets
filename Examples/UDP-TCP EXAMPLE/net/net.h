#ifndef __NET_H
#define __NET_H

#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

typedef struct {
	struct TCP {
	   in_port_t port_l, port_r;;  //local & remote
	   struct sockaddr_in addr_l, addr_r; //remote
	   int s;   //socket
	   char host[16];  //remote
	} TCP;

	struct UDP {
	   in_port_t port_l, port_r;
	   struct sockaddr_in addr_l, addr_r;
	   int s;
	   char host[16];
	} UDP;
} _net;

extern int TCP_socket_open( _net *p );
extern int UDP_socket_open( _net *p );
extern int Close( int s );
extern int Ready_Send_RST( int s );
extern ssize_t TCP_read( int s, void *ptr, size_t nbytes, int time_out );
extern ssize_t TCP_write( int s, void *ptr, size_t nbytes, int time_out );
extern ssize_t UDP_read( int s, void *ptr, size_t nbytes, struct sockaddr * addr_r );
extern ssize_t UDP_write( int s, void *ptr, size_t nbytes, struct sockaddr * addr_r );
extern int Signal_PIPE_Ignore();

#endif