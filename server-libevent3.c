// https://www.pacificsimplicity.ca/blog/libevent-echo-server-tutorial

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define UNIX_SOCK_PATH "/tmp/tempsocket"

static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t len = evbuffer_get_length(input);
    char *data;
    data = malloc(len);
    evbuffer_copyout(input, data, len);

    printf("we got some data: %s\n", data);

    /* Copy all the data from the input buffer to the output buffer. */
    evbuffer_add_buffer(output, input);
    free(data);
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener *listener, 
			   evutil_socket_t fd, 
			   struct sockaddr *address, 
			   int socklen, 
			   void *ctx)
{
    /* We got a new connection! Set up a bufferevent for it. */
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. "
            "Shutting down.\n", err, evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}




int main(int argc, char **argv)
{
  int TCP_port, baudRate;
  const char* serial_port;
  struct event_base *base;
  struct evconnlistener *listener;
  // struct sockaddr_un sin;	// specifies a unix socket (file)
  struct sockaddr_in sin;	// specifies a unix socket (file)

  // if commandline args are not set, then use defaults
  if ( argc < 4 ){
    TCP_port = 10000;
    baudRate = 9600;
    serial_port = "/dev/ttyS0";
  }
  
  else{
    TCP_port = atoi( argv[1] );
    baudRate = atoi( argv[2] );
    serial_port = argv[3];
  }

    
  printf("Server starting with the following parameters:\n");
  printf("tcp: %d, baud: %d, serial: %s\n",TCP_port,baudRate,serial_port);

  // Create new event base
  // ----------------------
  // http://www.wangafu.net/~nickm/libevent-2.0/doxygen/html/structevent__base.html
  // Structure to hold information and state for a Libevent dispatch loop.
  //  keeps track of all pending and active events, and notifies your application of the active ones.
  
  base = event_base_new();
  if (!base) {
    puts("Couldn't open event base");
    return 1;
  }
  
    // Clear the sockaddr before using it, in case there are extra
    //  platform-specific fields that can mess us up. 
    // note that this is using a unix socket, not a tcp port
  
    /*
      memset(&sin, 0, sizeof(sin));
      sin.sun_family = AF_LOCAL;
      strcpy(sin.sun_path, UNIX_SOCK_PATH);
    */
  
  // setup tcp socket
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(TCP_port);  // convert int to network byte order
  

  // Create a new listener 
  listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                       (struct sockaddr *) &sin, sizeof(sin));
    if (!listener) {
        perror("Couldn't create listener");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    /* Lets rock */
    event_base_dispatch(base);
    return 0;
}
