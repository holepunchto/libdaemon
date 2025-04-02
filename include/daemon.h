#ifndef DAEMON_H
#define DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int
daemon_spawn(const char *file, char *const argv[], char *const env[]);

#ifdef __cplusplus
}
#endif

#endif // DAEMON_H
