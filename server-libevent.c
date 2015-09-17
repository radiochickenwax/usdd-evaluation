/*
  Taken from low-level event.h tutorial.
  http://www.wangafu.net/~nickm/libevent-book/01_intro.html

  Note there's a lot more to be read in this book.  This is only the
  first chapter.  


#  Objective:
	Implement a simple serial TCP port server using C.  At
	startup, the program should open a serial port device (default
	/dev/ttyS0) and set the baud rate (default 9600 ).   It
	should also create a TCP socket and bind it to a port (default
	10000).  The program should then listen for connections on the
	TCP socket.

	While a client is connected any data received from the client
	should be written out to the serial port, and any data
	received from the serial port should be written back to the
	client.


#  Environment
	USDD will provide a shell account on a Linux development
	server with 2 serial ports. The serial ports will be connected
	together with a null modem cable.  
	
	(Explain what a null modem cable is).
	
	A terminal emulator program will be available for interacting
	with the serial connection.  USDD can install a text editor
	other than emacs, vi, or nano if requested. The program must
	be compiled using GCC and cannot be linked to any non-libc
	libraries other than libevent.  Use of Google, man pages, and
	other reference material is permitted and encouraged.

  
#  Requirements

	- only 1 TCP connection can be active at a time, subsequent
          connection requests should be refused.

	- The program's main loop use I/O multiplexing, either with
          the select/poll family of functions or libevent's
          event_dispatch function as the main loop.

	- The program should print messages to the console for events
          like "connection accepted" or "closed", "number of bytes
          sent or received" from the TCP socket or serial port, and
          any error messages.


	
#  Bonus Points

	- Allow the user to specify the serial port device, the baud
          rate, and the TCP port number using command line arguments
	- Use libevent instead of select/poll
	- Use non-blocking I/O (O_NONBLOCK) and handle buffering and
          partial writes (#  Bonus Feature

	Whenever the program receives an STX (0x02) byte over the TCP
	connection, assert the RTS line instead of writing it out to
	the serial port.  Similarly, when it receives an ETX (0x03)
	byte, de-assert the RTS line.

#  Deliverables:
  README.txt file, Makefile and all C source and header files

 */

/*
 
  gcc low-level-rot13-server-with-libevent.c -o low-level-rot13-server-with-libevent -levent
 */

/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MAX_LINE 16384

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

/*
  structure to store state.
  
  What gets stored in buffer exactly?  Lines?
  
 */
struct fd_state {
  char buffer[MAX_LINE];	// lines?
  size_t buffer_used;		// 
  
  size_t n_written;		// bytes written
  size_t write_upto;		// limit on bytes
  
  // event structs are explained at:  
  // (browse-url "http://www.wangafu.net/~nickm/libevent-2.0/doxygen/html/structevent.html")

  // alternatively at:
  /*
    (progn 
    (find-file-other-window "/usr/include/event2/event.h")
    (search-forward "@struct event"))
  */
  struct event *read_event;	
  struct event *write_event;
};

// allocate state?
// this needs better explanation
struct fd_state* alloc_fd_state(struct event_base *base, evutil_socket_t fd)
{
    struct fd_state *state = malloc(sizeof(struct fd_state));
    if (!state)
        return NULL;
    state->read_event = event_new(base, fd, EV_READ|EV_PERSIST, do_read, state);
    if (!state->read_event) {
        free(state);
        return NULL;
    }
    state->write_event =
        event_new(base, fd, EV_WRITE|EV_PERSIST, do_write, state);

    if (!state->write_event) {
        event_free(state->read_event);
        free(state);
        return NULL;
    }

    state->buffer_used = state->n_written = state->write_upto = 0;

    assert(state->write_event);
    return state;
}

// somewhat straight forward
void free_fd_state(struct fd_state *state)
{
    event_free(state->read_event);
    event_free(state->write_event);
    free(state);
}

// read from socket.
// do what with data?
void do_read(evutil_socket_t fd, short events, void *arg)
{
    struct fd_state *state = arg;  
    char buf[1024];
    int i;
    ssize_t result;
    while (1) {
        assert(state->write_event);
        result = recv(fd, buf, sizeof(buf), 0);
        if (result <= 0)
            break;

        for (i=0; i < result; ++i)  {
            if (state->buffer_used < sizeof(state->buffer))
	      // state->buffer[state->buffer_used++] = rot13_char(buf[i]);
	      state->buffer[state->buffer_used++] = buf[i];
            if (buf[i] == '\n') {
                assert(state->write_event);
                event_add(state->write_event, NULL);
                state->write_upto = state->buffer_used;
            }
        }
    }

    if (result == 0) {
        free_fd_state(state);
    } else if (result < 0) {
        if (errno == EAGAIN) // XXXX use evutil macro
            return;
        perror("recv");
        free_fd_state(state);
    }
}

void do_write(evutil_socket_t fd, short events, void *arg)
{
    struct fd_state *state = arg;

    while (state->n_written < state->write_upto) {
        ssize_t result = send(fd, state->buffer + state->n_written,
                              state->write_upto - state->n_written, 0);
        if (result < 0) {
            if (errno == EAGAIN) // XXX use evutil macro
                return;
            free_fd_state(state);
            return;
        }
        assert(result != 0);

        state->n_written += result;
    }

    if (state->n_written == state->buffer_used)
        state->n_written = state->write_upto = state->buffer_used = 1;

    event_del(state->write_event);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) { // XXXX eagain??
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd); // XXX replace all closes with EVUTIL_CLOSESOCKET */
    } else {
        struct fd_state *state;
        evutil_make_socket_nonblocking(fd);
        state = alloc_fd_state(base, fd);
        assert(state); /*XXX err*/
        assert(state->write_event);
        event_add(state->read_event, NULL);
	printf("Connection accepted\n");
    }
}

void run(int portNo)
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    base = event_base_new();
    if (!base)
        return; /*XXXerr*/

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(portNo);  // convert int to network byte order

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

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
}

int main(int argc, char **argv)
{
  int TCP_port, baudRate;
  const char* serial_port;
  
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
    
  printf("Server started with the following parameters:\n");
  printf("tcp: %d, baud: %d, serial: %s\n",TCP_port,baudRate,serial_port);

  setvbuf(stdout, NULL, _IONBF, 0);	// what?
  /* 
     get TCP port number, 
   */

  run(TCP_port);				// event loop
  return 0;				// exit
}
