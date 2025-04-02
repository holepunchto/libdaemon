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
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[]) {
  int fd[2];

  if (pipe(fd) < 0) return -1;

  umask(0);

  struct rlimit rl;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) abort();

  if (rl.rlim_max == RLIM_INFINITY) rl.rlim_max = 1024;

  pid_t pid;

  pid = fork();

  if (pid < 0) return -1;

  if (pid != 0) {
    close(fd[1]);

    if (read(fd[0], &daemon->pid, sizeof(daemon->pid)) != sizeof(daemon->pid)) {
      close(fd[0]);

      return -1;
    }

    close(fd[0]);

    int stat;

    waitpid(pid, &stat, 0);

    return 0;
  }

  setsid();

  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;

  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGHUP, &sa, NULL) < 0) abort();

  pid = fork();

  if (pid < 0) abort();

  if (pid != 0) {
    close(fd[0]);

    write(fd[1], &pid, sizeof(pid));
    close(fd[1]);

    exit(0);
  }

  close(fd[0]);
  close(fd[1]);

  for (int i = 0; i < rl.rlim_max; i++) {
    close(i);
  }

  int fd0, fd1, fd2;

  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  return execve(file, argv, env);
}
