#include "net.h"

#include <fcntl.h>
#include <pthread.h>

int Signal_PIPE_Ignore() {
	signal( SIGPIPE, SIG_IGN );	
	return 0;
};

int TCP_socket_open( _net *p ) {
	int on = 1;

	memset( &p->TCP.addr_l, 0, sizeof( p->TCP.addr_l ) );
	p->TCP.addr_l.sin_len = sizeof( p->TCP.addr_l );
	p->TCP.addr_l.sin_family = AF_INET;
	p->TCP.addr_l.sin_port = htons( p->TCP.port_l );
	p->TCP.addr_l.sin_addr.s_addr = htonl( INADDR_ANY );

	if( ( p->TCP.s = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) return -1;

    if( setsockopt( p->TCP.s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof( on ) ) != 0 ) return -2;

	//if( bind( p->TCP.s, (struct sockaddr*) &p->TCP.addr_l, sizeof( p->TCP.addr_l ) )  != 0 ) return -3;

    memset(&p->TCP.addr_r, 0, sizeof( p->TCP.addr_r ) );
    p->TCP.addr_r.sin_len = sizeof( p->TCP.addr_r );
    p->TCP.addr_r.sin_family = AF_INET;
    p->TCP.addr_r.sin_port = htons( p->TCP.port_r );
    if( inet_aton( p->TCP.host, &p->TCP.addr_r.sin_addr ) != 1) { errno = EINVAL; return -4; };

    if( connect( p->TCP.s, (struct sockaddr *) &p->TCP.addr_r, sizeof( p->TCP.addr_r ) ) != 0 ) return -5;

	return 0;
};

int TCP_socket_open_tolisten( _net *p ) {
	int on = 1;

    memset(&p->TCP.addr_l, 0, sizeof( p->TCP.addr_l ) );
    p->TCP.addr_l.sin_len = sizeof( p->TCP.addr_l );
    p->TCP.addr_l.sin_family = AF_INET;
    p->TCP.addr_l.sin_port = htons( p->TCP.port_l );
    p->TCP.addr_l.sin_addr.s_addr = htonl( INADDR_ANY );

	if( ( p->TCP.s = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) return -1;

	if( setsockopt( p->TCP.s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof( on ) ) != 0 ) return -2;
	if( setsockopt( p->TCP.s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof( on ) ) != 0 ) return -3;

	if( bind( p->TCP.s, (struct sockaddr*) &p->TCP.addr_l, sizeof( p->TCP.addr_l ) )  != 0 ) return -4;

    if( listen( p->TCP.s, 1 ) != 0 ) return -5;

	return 0;
};

int UDP_socket_open( _net *p ) {

	memset( &p->UDP.addr_l, 0, sizeof( p->UDP.addr_l ) );
	p->UDP.addr_l.sin_len = sizeof( p->UDP.addr_l );
	p->UDP.addr_l.sin_family = AF_INET;
	p->UDP.addr_l.sin_port = htons( p->UDP.port_l );
	p->UDP.addr_l.sin_addr.s_addr = htonl( INADDR_ANY );

	if( (p->UDP.s  = socket( AF_INET, SOCK_DGRAM, 0 ) ) == -1 ) return -1;

	if( bind( p->UDP.s, (struct sockaddr*) &p->UDP.addr_l, sizeof( p->UDP.addr_l ) )  != 0 ) return -2;

    memset(&p->UDP.addr_r, 0, sizeof( p->UDP.addr_r ) );
    p->UDP.addr_r.sin_len = sizeof( p->UDP.addr_r );
    p->UDP.addr_r.sin_family = AF_INET;
    p->UDP.addr_r.sin_port = htons( p->UDP.port_r );
    if( inet_aton( p->UDP.host, &p->UDP.addr_r.sin_addr ) != 1) { errno = EINVAL; return -3; };
	
    //if( connect( p->UDP.s, (struct sockaddr *) &p->UDP.addr_r, sizeof( p->UDP.addr_r ) ) != 0 ) return -4;

	return 0;
};

int Close( int s ) {
	int rc;

	for(;;) {
		rc = close( s );
		if( rc == 0 ) break;  // OK
		if( errno == EINTR ) continue; else return -1;
	};
	return 0;
};

int Ready_Send_RST( int s ) {
	struct linger lin;
		
	lin.l_onoff = 1;
	lin.l_linger = 0;
	if( setsockopt( s, SOL_SOCKET, SO_LINGER, &lin, sizeof( lin ) ) != 0 ) return -1;
	return 0;
};

ssize_t TCP_read( int s, void *ptr, size_t nbytes, int time_out  ) {
//not debug with select()
	fd_set fdset;
    struct timeval tv, *ptv;
    int n; 
	ssize_t nread;
    size_t nleft = nbytes;
    char* p = ptr;
    
	FD_ZERO( &fdset );
    FD_SET( s, &fdset );
 
    if( time_out > 0 ) { tv.tv_sec = time_out; tv.tv_usec = 0; ptv = &tv; } else ptv = NULL;
 
	for(;;) {
		n = select( s + 1, &fdset, NULL, NULL, ptv );
		if( n < 0 && errno == EINTR ) continue;
		break;
	};
	
    if( n < 0 ) return -1;   //error in select
    if( n == 0 ) { errno = ETIME; return -2; }; //time_out in read from socket

	for(;;) {
		nread = recv(s, p, nleft, 0);
		if (nread >= 0) break;
		if (errno == EINTR) continue; else return -3; 
	};
	
    return nread;
};

ssize_t TCP_write( int s, void *ptr, size_t nbytes, int time_out ) {
	fd_set fdset;
    struct timeval tv, *ptv;
    int n; 
	size_t	nleft = nbytes;
	ssize_t  nwritten;
	char* p = ptr;
    
	if (nleft == 0 || p == NULL ) return 0;  //empty buf

	FD_ZERO( &fdset );
    FD_SET( s, &fdset );
 
    if( time_out > 0 ) { tv.tv_sec = time_out; tv.tv_usec = 0; ptv = &tv; } else ptv = NULL;
 
	for(;;) {
		n = select( s + 1, NULL, &fdset, NULL, ptv );
		if( n < 0 && errno == EINTR ) continue;
		break;
	};
	
    if( n < 0 ) return -1;   //error in select
    if( n == 0 ) { errno = ETIME; return -2; }; //time_out in write to socket
/*
	while( nleft > 0 ) {
		nwritten = write( s, p, nleft );
		if( nwritten < 0 && errno == EINTR ) continue;
		if( nwritten < 0 && errno != EINTR ) return -4;
		if( nwritten == 0 ) { errno = EIO; return -5; };
		nleft -= nwritten;
 		p     += nwritten;
	};
	nwritten = nbytes;
*/
	while( nleft > 0 ) {
		nwritten = send( s, p, nleft, 0 );
		if( nwritten < 0 ) return -4;
		if( nwritten == 0 ) { errno = EIO; return -5; };
		nleft -= nwritten;
 		p     += nwritten;
	};
	nwritten = nbytes;

	return nwritten;
};

ssize_t UDP_read( int s, void *ptr, size_t nbytes, struct sockaddr * addr_r ) {
    int size = sizeof( *addr_r );
	ssize_t nread;

	for(;;) {
		nread = recvfrom(s, ptr, nbytes, 0, addr_r, &size);
		if (nread >= 0) break;
		if (errno == EINTR) continue; else return -1; 
	};
	
    return nread;
};

ssize_t UDP_write( int s, void *ptr, size_t nbytes, struct sockaddr * addr_r ) {
	size_t	nleft = nbytes;
	ssize_t  nwritten;
	char* p = ptr;
    int size = sizeof( *addr_r );
    
	while( nleft > 0 ) {
		nwritten = sendto(s, p, nleft, 0, addr_r, size);
		if( nwritten < 0 ) return -4;
		if( nwritten == 0 ) { errno = EIO; return -5; };
		nleft -= nwritten;
 		p     += nwritten;
	};
	nwritten = nbytes;

	return nwritten;
};

void* TCP_pthread(void* pp) {
	int rc, i;
	_net* p = (_net*)pp;
#define SIZE 1024
    int f, size_read = SIZE;
    char buf[SIZE];

    printf("TCP thread BEGIN\n");

	f = open("test.txt", O_RDONLY);

	rc = TCP_socket_open(p);
	if( rc < 0 ) { printf("TCP_socket_open rc=%i errno=%i\n", rc, errno); return NULL; };
	
	i = 1; errno = 0;
	while(i){
	    size_read = read( f, buf, sizeof( buf ) );
	    if( size_read == -1 ) { perror( "Error reading file\n" );	break; }
	    if( size_read != SIZE ) { i = 0; }

		rc=TCP_write(p->TCP.s, buf, size_read, 10 );
		if(rc < 0) { printf("rc=%i, errno = %i\n", rc, errno); break; };
//		rc=TCP_read(p->TCP.s, buf, size_read );
//		buf[rc] = '\0'; printf("rc=%i, errno = %i, s= %s\n", rc, errno, buf);
//		if(rc <= 0)  break;
	};

	if( rc < 0 ) Ready_Send_RST( p->TCP.s );  //++ control return from Ready_Send_RST()
	rc = Close(p->TCP.s);
	if( rc < 0 ) { printf("TCP_socket_close rc=%i\n", rc);  return NULL; };

    Close( f );	

    printf("TCP thread END\n");
	return NULL;	
};

void* UDP_pthread(void* pp) {
	int rc, i, k;
	char sUDP[10000];
	char rUDP[20];
	_net* p = (_net*)pp;

    printf("UDP thread BEGIN\n");

	rc = UDP_socket_open(p);
	if( rc < 0 ) { printf("UDP_socket_open rc=%i errno=%i\n", rc, errno); return NULL; };

	for(i=0; i<1;i++){
		errno = 0;
        rc=UDP_read( p->UDP.s, rUDP, sizeof(rUDP)-1, (struct sockaddr *) &p->UDP.addr_r );
		if(rc > 0) rUDP[rc]= '\0';
		printf("UDPrecvfrom  rc=%i, err=%i, s=%s, from=%s\n", rc, errno, rUDP, inet_ntoa(p->UDP.addr_r.sin_addr));

        for(k=0; k<100; k++) { sUDP[k] = '1'; }; sUDP[k] = '\0';
        rc=UDP_write(p->UDP.s, sUDP, strlen(sUDP), (struct sockaddr *) &p->UDP.addr_r );
		printf("UDP sendto rc=%i, err=%i, s=%s, to=%s\n", rc, errno, sUDP, inet_ntoa(p->UDP.addr_r.sin_addr));
	};

	rc = Close(p->UDP.s);
	if( rc < 0 ) { printf("UDP_socket_close rc=%i\n", rc);  return NULL; };
	
    printf("UDP thread END\n");

	return NULL;	
};

int main(int argc, char *argv[]) {
	_net c;
	int rc;
	pthread_t thread1;
//    pthread_t thread2;
	
	printf("Welcome to the Sockets API\n");

	c.TCP.port_l = 9000; 
	c.TCP.port_r = 9000; 
	strcpy(c.TCP.host, "10.0.136.9"); 
	c.UDP.port_l = 9000; 
	c.UDP.port_r = 9000; 
	strcpy(c.UDP.host, "0.0.0.0");  //init so
	
	Signal_PIPE_Ignore();	

	rc = pthread_create(&thread1, NULL, TCP_pthread, &c);
	if( rc != 0 ) { printf("TCP_pthread_create rc=%i\n", rc); return 0; };
	
//    rc = pthread_create(&thread2, NULL, UDP_pthread, &c);
//    if( rc != 0 ) { printf("UDP_pthread_create rc=%i\n", rc); return 0; };

    rc = pthread_join(thread1, NULL);
	if( rc != 0 ) { printf("TCP_pthread_join rc=%i\n", rc); /*return 0;*/ };

//    rc = pthread_join(thread2, NULL);
//    if( rc != 0 ) { printf("UDP_pthread_join rc=%i\n", rc); return 0; };

	printf("EXIT_SUCCESS\n");

	return EXIT_SUCCESS;
};
