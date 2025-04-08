#include <assert.h>
#include <daemon.h>
#include <stdio.h>

int
main() {
  int e;

  daemon_t daemon;
  e = daemon_spawn(&daemon, "./this-does-not-exist", NULL, NULL);
  assert(e != 0);

  printf("daemon.pid=%d\n", daemon.pid);
}
