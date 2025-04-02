#include <process.h>
#include <windows.h>

#include "../include/daemon.h"

int
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[]) {
  const char *default_argv[2] = {file, NULL};

  if (argv == NULL) argv = default_argv;

  intptr_t handle = _spawnve(_P_NOWAIT, file, argv, env);

  if (handle == -1) return -1;

  daemon->pid = GetProcessId((HANDLE) handle);

  CloseHandle((HANDLE) handle);

  return 0;
}
