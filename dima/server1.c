/*
 * РЁР°Р±Р»РѕРЅ РїРѕСЃР»РµРґРѕРІР°С‚РµР»СЊРЅРѕРіРѕ СЌС…Рѕ-СЃРµСЂРІРµСЂР° TCP.
 *
 * РљРѕРјРїРёР»СЏС†РёСЏ:
 *	cc -Wall -O2 -o server1 server1.c
 */

#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * РљРѕРЅС„РёРіСѓСЂР°С†РёСЏ СЃРµСЂРІРµСЂР°.
 */
#define PORT 1027
#define BACKLOG 5
#define MAXLINE 256

#define SA struct sockaddr

/*
 * РћР±СЂР°Р±РѕС‚С‡РёРє С„Р°С‚Р°Р»СЊРЅС‹С… РѕС€РёР±РѕРє.
 */
void error(const char *s)
{
	perror(s);
	exit(-1);
}

/*
 * Р¤СѓРЅРєС†РёРё-РѕР±РµСЂС‚РєРё.
 */
int Socket(int domain, int type, int protocol)
{
	int rc;
	
	rc = socket(domain, type, protocol);
	if(rc == -1) error("socket()");

	return rc;
}

int Bind(int socket, struct sockaddr *addr, socklen_t addrlen)
{
	int rc;
	
	rc = bind(socket, addr, addrlen);
	if(rc == -1) error("bind()");

	return rc;
}

int Listen(int socket, int backlog)
{
	int rc;
	
	rc = listen(socket, backlog);
	if(rc == -1) error("listen()");

	return rc;
}

int Accept(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
	int rc;
	
	for(;;) {
		rc = accept(socket, addr, addrlen);
		if(rc != -1) break;
		if(errno == EINTR || errno == ECONNABORTED) continue;
		error("accept()");
	}		

	return rc;	
}

void Close(int fd)
{
	int rc;
	
	for(;;) {
		rc = close(fd);
		if(!rc) break;
		if(errno == EINTR) continue;
		error("close()");
	}
}

size_t Read(int fd, void *buf, size_t count)
{
	ssize_t rc;
	
	for(;;) {
		rc = read(fd, buf, count);
		if(rc != -1) break;
		if(errno == EINTR) continue;
		error("read()");
	}
	
	return rc;
}

size_t Write(int fd, const void *buf, size_t count)
{
	ssize_t rc;
	
	for(;;) {
		rc = write(fd, buf, count);
		if(rc != -1) break;
		if(errno == EINTR) continue;
		error("write()");
	}
	
	return rc;
}

/*
 * Р§С‚РµРЅРёРµ СЃС‚СЂРѕРєРё РёР· СЃРѕРєРµС‚Р°.
 */
size_t reads(int socket, char *s, size_t size)
{
	char *p;
	size_t n, rc;
	
	/* РџСЂРѕРІРµСЂРёС‚СЊ РєРѕСЂСЂРµРєС‚РЅРѕСЃС‚СЊ РїРµСЂРµРґР°РЅРЅС‹С… Р°СЂРіСѓРјРµРЅС‚РѕРІ. */
	if(s == NULL) {
		errno = EFAULT;
		error("reads()");
	}
	if(!size) return 0;

	p = s;
	size--;
	n = 0;
	while(n < size) {
		rc = Read(socket, p, 1);
		if(rc == 0) break;
		if(*p == '\n') {
			p++;
			n++;
			break;
		}
		p++;
		n++;
	}
	*p = 0;
	
	return n;
}

/*
 * Р—Р°РїРёСЃСЊ count Р±Р°Р№С‚РѕРІ РІ СЃРѕРєРµС‚.
 */
size_t writen(int socket, const char *buf, size_t count)
{
	const char *p;
	size_t n, rc;

	/* РџСЂРѕРІРµСЂРёС‚СЊ РєРѕСЂСЂРµРєС‚РЅРѕСЃС‚СЊ РїРµСЂРµРґР°РЅРЅС‹С… Р°СЂРіСѓРјРµРЅС‚РѕРІ. */
	if(buf == NULL) {
		errno = EFAULT;
		error("writen()");
	}
	
	p = buf;
	n = count;
	while(n) {
		rc = Write(socket, p, n);
		n -= rc;
		p += rc;
	}

	return count;
}

void serve_client(int socket)
{
	char s[MAXLINE];
	ssize_t rc;

	while((rc = reads(socket, s, MAXLINE)) > 0) {
		if(writen(socket, s, rc) == -1) break;
	}
	Close(socket);	
}

int udpSendTime (int socket, unsigned int flags, const struct sockaddr *to, int tolen)
 {
	int total;
	time_t t = time(NULL);
	struct tm *local;
	local = localtime(&t);
	
	for(;;){
		total = sendto(socket, asctime(local), sizeof(asctime(local)), flags, to, tolen);
		if(total != -1) break;
		if(errno == EINTR) continue;
		error("write()");
	}
	
	return total;
 }

int main(void)
{
	int lsocket;	/* Р”РµСЃРєСЂРёРїС‚РѕСЂ РїСЂРѕСЃР»СѓС€РёРІР°РµРјРѕРіРѕ СЃРѕРєРµС‚Р°. */
	int csocket;	/* Р”РµСЃРєСЂРёРїС‚РѕСЂ РїСЂРёСЃРѕРµРґРёРЅРµРЅРЅРѕРіРѕ СЃРѕРєРµС‚Р°. */
	struct sockaddr_in servaddr;
	
	/* РЎРѕР·РґР°С‚СЊ СЃРѕРєРµС‚. */
	lsocket = Socket(PF_INET, SOCK_STREAM, 0);

	/* РРЅРёС†РёР°Р»РёР·РёСЂРѕРІР°С‚СЊ СЃС‚СЂСѓРєС‚СѓСЂСѓ Р°РґСЂРµСЃР° СЃРѕРєРµС‚Р° СЃРµСЂРІРµСЂР°. */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* РЎРІСЏР·Р°С‚СЊ СЃРѕРєРµС‚ СЃ Р»РѕРєР°Р»СЊРЅС‹Рј Р°РґСЂРµСЃРѕРј РїСЂРѕС‚РѕРєРѕР»Р°. */
	Bind(lsocket, (SA *) &servaddr, sizeof(servaddr));

	udpSendTime(lsocket, 0, (SA *) $servaddr, sizeof(servaddr));
	
	Close(lsocket);
	/* РџСЂРµРѕР±СЂР°Р·РѕРІР°С‚СЊ РЅРµРїСЂРёСЃРѕРµРґРёРЅРµРЅРЅС‹Р№ СЃРѕРєРµС‚ РІ РїР°СЃСЃРёРІРЅС‹Р№. */
	//Listen(lsocket, BACKLOG);
			
	//for(;;) {
	//	csocket = Accept(lsocket, NULL, 0);
	//	serve_client(csocket);
	//}
	
	return 0;
}