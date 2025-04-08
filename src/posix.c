#include <errno.h>
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

    ssize_t n;

    n = read(fd[0], &daemon->pid, sizeof(daemon->pid));

    if (n != sizeof(daemon->pid)) {
      close(fd[0]);

      waitpid(pid, NULL, 0);

      return -1;
    }

    int err;

    n = read(fd[0], &err, sizeof(err));

    close(fd[0]);

    waitpid(pid, NULL, 0);

    if (n == sizeof(err)) {
      errno = -err;

      return -1;
    }

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

  if (pid != 0) _exit(0);

  pid = getpid();

  close(fd[0]);

  fcntl(fd[1], F_SETFD, FD_CLOEXEC);

  write(fd[1], &pid, sizeof(pid));

  for (int i = 0; i < rl.rlim_max; i++) {
    if (i != fd[1]) close(i);
  }

  int io = open("/dev/null", O_RDWR);

  if (io >= 0) {
    dup2(io, 0);
    dup2(io, 1);
    dup2(io, 2);

    if (io > 2) close(io);
  }

  execve(file, (char *const *) argv, (char *const *) env);

  int err = errno;

  write(fd[1], &err, sizeof(err));
  close(fd[1]);

  _exit(127);
}
