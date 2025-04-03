#include <stdio.h>
#include <windows.h>

#include "../include/daemon.h"

int
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[]) {
  STARTUPINFOA si;
  ZeroMemory(&si, sizeof(si));

  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  char cmd[MAX_PATH];
  snprintf(cmd, sizeof(cmd), "\"%s\"", file);

  BOOL success = CreateProcessA(
    NULL,
    cmd,
    NULL,
    NULL,
    FALSE,
    CREATE_NO_WINDOW | DETACHED_PROCESS,
    NULL,
    NULL,
    &si,
    &pi
  );

  if (!success) return -1;

  CloseHandle(pi.hThread);

  daemon->pid = (int) pi.dwProcessId;

  return 0;
}
