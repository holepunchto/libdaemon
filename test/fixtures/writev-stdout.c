#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

int
main(void) {
  close(1);
  open("/dev/null", O_RDWR);

  struct iovec iov[3];

  char *msg1 = "Hello ";
  char *msg2 = "from ";
  char *msg3 = "writev!\n";

  iov[0].iov_base = msg1;
  iov[0].iov_len = strlen(msg1);

  iov[1].iov_base = msg2;
  iov[1].iov_len = strlen(msg2);

  iov[2].iov_base = msg3;
  iov[2].iov_len = strlen(msg3);

  ssize_t written = writev(STDOUT_FILENO, iov, 3);

  return written;
}
