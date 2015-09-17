#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(const char* serial_port)
{
  int fd; /* File descriptor for the port */


  fd = open(serial_port, O_RDWR | O_NONBLOCK | O_NDELAY);
  if (fd == -1) // error
    perror("open_port: Unable to open specified port\n");
  else
    fcntl(fd, F_SETFL, 0);

  return (fd);
}

int read_port(const char* serial_port)
{
  int fd = open(serial_port, O_RDONLY | O_NOCTTY);
  if (fd == -1)
    {
      /* Could not open the port. */
      perror("open_port: Unable to open specified port  ");
    }
  
  char buffer[32];
  int n = read(fd, buffer, sizeof(buffer));
  if (n < 0)
    fputs("read failed!\n", stderr);
  return (fd);
}


int main(int argc, char** argv)
{
  const char* serial_port = "/dev/ttyS0";
  int fd =  open_port(serial_port);
  return 0;
}
