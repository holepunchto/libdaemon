#include <process.h>

#include "../include/daemon.h"

int
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[]) {
  intptr_t handle = _spawnve(_P_NOWAIT, file, argv, env);

  if (handle == -1) return -1;

  daemon->pid = (int) handle;

  return 0;
}
