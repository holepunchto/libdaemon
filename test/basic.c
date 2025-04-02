#include <assert.h>
#include <daemon.h>

int
main() {
  int e;
  e = daemon_spawn(FIXTURE_SLEEP, NULL, NULL);
  assert(e == 0);
}
