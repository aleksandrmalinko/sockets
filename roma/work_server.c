/*
 * Шаблон параллельного эхо-сервера TCP, работающего по модели
 * "один клиент - один процесс".
 *
 * Компиляция:
 *	cc -Wall -O2 -o server2 server2.c
 */

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Конфигурация сервера.
 */
#define PORT 1027
#define BACKLOG 5
#define MAXLINE 256

#define SA struct sockaddr

/*
 * Обработчик фатальных ошибок.
 */
void error(const char *s)
{
    perror(s);
    exit(-1);
}

/*
 * Функции-обертки.
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

void Sigaction(int signum, const struct sigaction *act, struct sigaction *oact)
{
    int rc;

    for(;;) {
        rc = sigaction(signum, act, oact);
        if(!rc) break;
        if(errno == EINTR) continue;
        error("close()");
    }
}

pid_t Fork()
{
    pid_t rc;

    rc = fork();
    if(rc == -1) error("fork()");

    return rc;
}

/*
 * Чтение строки из сокета.
 */
size_t reads(int socket, char *s, size_t size)
{
    char *p;
    size_t n, rc;

    /* Проверить корректность переданных аргументов. */
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
 * Запись count байтов в сокет.
 */
size_t writen(int socket, const char *buf, size_t count)
{
    const char *p;
    size_t n, rc;

    /* Проверить корректность переданных аргументов. */
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

/*
 * Обработчик сигнала SIGCHLD.
 */
static void sighandler(int signum)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * Установить обработчик сигнала SIGCHLD.
 */
static void set_sighandler()
{
    struct sigaction act;

    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    Sigaction(SIGCHLD, &act, NULL);
}

int main()
{
    int lsocket;	/* Дескриптор прослушиваемого сокета. */
    int csocket;	/* Дескриптор присоединенного сокета. */
    struct sockaddr_in servaddr;
    pid_t pid;

    /* Установить обработчик сигнала SIGCHLD. */
    set_sighandler();

    /* Создать сокет. */
    lsocket = Socket(PF_INET, SOCK_STREAM, 0);

    /* Инициализировать структуру адреса сокета сервера. */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Связать сокет с локальным адресом протокола. */
    Bind(lsocket, (SA *) &servaddr, sizeof(servaddr));

    /* Преобразовать неприсоединенный сокет в пассивный. */
    Listen(lsocket, BACKLOG);

    for(;;) {
        csocket = Accept(lsocket, NULL, 0);

        pid = Fork();
        if(pid) {
            /* Родительский процесс. */
            Close(csocket);
        }
        else {
            /* Дочерний процесс. */
            Close(lsocket);
            serve_client(csocket);
            exit(0);
        }

    }

    return 0;
}
