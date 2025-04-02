#ifndef DAEMON_H
#define DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#if defined(_WIN32)
#include "daemon/win32.h"
#else
#include "daemon/posix.h"
#endif

typedef struct daemon_s daemon_t;

struct daemon_s {
  daemon_pid_t pid;
};

int
daemon_spawn(daemon_t *daemon, const char *file, char *const argv[], char *const env[]);

#ifdef __cplusplus
}
#endif

#endif // DAEMON_H
