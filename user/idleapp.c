// idleapp: a tiny do-nothing "app" used by launcher as a victim candidate
// for the Low-Memory Killer. It holds a little memory and sleeps forever.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char *m = sbrk(64 * 1024);
  if (m != SBRK_ERROR) {
    for (int off = 0; off < 64 * 1024; off += 4096)
      m[off] = 1;
  }
  for (;;)
    pause(10);
}
