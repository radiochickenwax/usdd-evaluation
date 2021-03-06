
#  Objective:
	Implement a simple serial TCP port server using C.  At
	startup, the program should open a serial port device (default
	/devb/ttyS0) and set the baud rate (default 9600 ).   It
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
          partial writes (EAGAIN).


#  Bonus Feature

	Whenever the program receives an STX (0x02) byte over the TCP
	connection, assert the RTS line instead of writing it out to
	the serial port.  Similarly, when it receives an ETX (0x03)
	byte, de-assert the RTS line.

#  Deliverables:
  README.txt file, Makefile and all C source and header files

