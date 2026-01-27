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
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[], const char *cwd) {
  int err;

  int fd[2];
  err = pipe(fd);
  if (err < 0) return -1;

  struct rlimit rl;
  err = getrlimit(RLIMIT_NOFILE, &rl);
  if (err < 0) return -1;

  if (rl.rlim_max == RLIM_INFINITY) rl.rlim_max = 1024;

  pid_t pid = fork();
  if (pid < 0) return -1;
  if (pid != 0) {
    close(fd[1]);

    ssize_t n = read(fd[0], &daemon->pid, sizeof(daemon->pid));

    if (n != sizeof(daemon->pid)) {
      close(fd[0]);
      waitpid(pid, NULL, 0);
      return -1;
    }

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

  err = sigaction(SIGHUP, &sa, NULL);
  if (err < 0) goto err;

  pid = fork();
  if (pid < 0) abort();
  if (pid != 0) _exit(0);

  close(fd[0]);
  fcntl(fd[1], F_SETFD, FD_CLOEXEC);

  pid = getpid();
  write(fd[1], &pid, sizeof(pid));

  for (int i = 0; i < rl.rlim_max; i++) {
    if (i != fd[1]) close(i);
  }

  if (cwd) {
    err = chdir(cwd);
    if (err < 0) goto err;
  }

  err = open("/dev/null", O_RDONLY);
  if (err < 0) goto err;

  int nul_in = err;

  err = open("/dev/null", O_WRONLY);
  if (err < 0) {
    close(nul_in);

    goto err;
  }

  int nul_out = err;

  dup2(nul_in, STDIN_FILENO);
  dup2(nul_out, STDOUT_FILENO);
  dup2(nul_out, STDERR_FILENO);

  if (nul_in > STDERR_FILENO) close(nul_in);
  if (nul_out > STDERR_FILENO) close(nul_out);

  execve(file, (char *const *) argv, (char *const *) env);

err:
  err = errno;
  write(fd[1], &err, sizeof(err));
  close(fd[1]);

  _exit(1);
}
