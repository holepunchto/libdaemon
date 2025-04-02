#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/daemon.h"

// See "Advanced Programming in the UNIX® Environment" by W. Richard Stevens and
// Stephen A. Rago for more information on the double-fork technique.

int
daemon_spawn(const char *file, char *const argv[], char *const env[]) {
  umask(0);

  struct rlimit rl;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) abort();

  if (rl.rlim_max == RLIM_INFINITY) rl.rlim_max = 1024;

  pid_t pid;

  pid = fork();

  if (pid < 0) return -1;

  if (pid != 0) return 0;

  setsid();

  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;

  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGHUP, &sa, NULL) < 0) abort();

  pid = fork();

  if (pid < 0) abort();

  if (pid != 0) exit(0);

  for (int i = 0; i < rl.rlim_max; i++) {
    close(i);
  }

  int fd0, fd1, fd2;

  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  return execve(file, argv, env);
}
