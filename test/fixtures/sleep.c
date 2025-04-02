#if defined(_WIN32)
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#endif

int
main() {
  sleep(10);
}
