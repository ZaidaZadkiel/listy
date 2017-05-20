/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "listy5.h"
#include <arpa/inet.h>

#define MAX_LINE 16384
#define MAX_CLIENTS 10

#define LOCAL_PORT 9999

env *e = NULL;
int clients[MAX_CLIENTS];
int new_client_id = 0;

void do_read (evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

char rot13_char(char c) {
    /* We don't want to use isalpha here; setting the locale would change
     * which characters are considered alphabetical. */
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

void process(char *line, char *buffer, int buff_len){
  printf("process: %s\n", line );
  
  render_list(
    eval(line),
    buffer,
    buff_len
  );
;;
}

void readcb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input, *output;
    char  *line;
    size_t n;
    int    i;
    
    input  = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    char buffer[2014];
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
        process     (line, buffer, 1024);
        printf("buffer: %s", buffer);
        //evbuffer_add(output, line, n);
        evbuffer_add(output, buffer, strlen(buffer));
        evbuffer_add(output, "\n", 1);
        free        (line);
    }

    if (evbuffer_get_length(input) >= MAX_LINE) {
        /* Too long; just process what there is and go on so that the buffer
         * doesn't grow infinitely long. */
        char buf[1024];
        while (evbuffer_get_length(input)) {
            int n = evbuffer_remove(input, buf, sizeof(buf));
            process     (line, buf, 1024);
            evbuffer_add(output, buf, n);
        }
        evbuffer_add(output, "\n", 1);
    } //if (evbuffer_get_length() >= MAX_LINE
} // void readcb()

void errorcb(struct bufferevent *bev, short error, void *ctx) {
    if (error & BEV_EVENT_EOF) {
        /* connection has been closed, do any clean up here */
        /* ... */
        //struct evbuffer *input = bufferevent_get_input(bev);
        //printf("bev fd=%d\n", evb input. fd);
        printf("error: BEV_EVENT_EOF\n");
    } else if (error & BEV_EVENT_ERROR) {
        /* check errno to see what error occurred */
        /* ... */
        printf("error: BEV_EVENT_ERROR\n");
    } else if (error & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
        printf("error: BEV_EVENT_TIMEOUT\n");
    }
    bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
    struct sockaddr_storage ss;
    struct event_base *base = arg;
    socklen_t          slen = sizeof(ss);
    int                fd   = accept(listener, (struct sockaddr*)&ss, &slen);
    
    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        //printable ip address
        char ipaddr[INET6_ADDRSTRLEN];
        getpeername(fd,        (struct sockaddr*)    &ss,            &slen);
        inet_ntop  (AF_INET, &((struct sockaddr_in *)&ss)->sin_addr, ipaddr, sizeof ipaddr);
        
        // find the first open slot (clients[i] == 0) and use it for the accepted client
        int i = 0;
        while(clients[i] != 0) i++;
        //set the found slot as the clients file-descriptor for later use
        clients[i] = fd;// new_client_id++;// &((struct sockaddr_in *)&ss)->sin_addr;
        printf("client #%d: IP is %s\n", clients[i], ipaddr);
        
        //initialize bufferevent with the socket file descriptor connection
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd); //allows working in the background
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    } // if(fd <0)
}// do_accept(...)

void run(void) {
    evutil_socket_t     listener;
    struct sockaddr_in  sin;
    struct event_base  *base;
    struct event       *listener_event;

    base = event_base_new();
    if (!base) return; /*XXXerr*/

    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port        = htons(LOCAL_PORT); //port address to listen to

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

#ifndef WIN32
    {
      int one = 1;
      setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
#endif

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
      perror("bind");
      return;
    }

    if (listen(listener, 16)<0) {
      perror("listen");
      return;
    }

    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    /*XXX check it */
    event_add(listener_event, NULL);

    event_base_dispatch(base);
} // void run()

int main(int c, char **v) {
    setvbuf(stdout, NULL, _IONBF, 0);
    for(int i =0; i != MAX_CLIENTS; i++) clients[i] = 0;

    printf("listy6 server with libevent2\n2016 by ZaidaZadkiel\nListening on localhost port %d\n\n", LOCAL_PORT);
    run();
    return 0;
}
