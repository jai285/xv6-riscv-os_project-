// memhog: stress test for the Low-Memory Killer (LMK).
// Marks itself BACKGROUND (FOREGROUND apps are never killed), then keeps
// grabbing and touching memory until the LMK wakes up and kills it.
//
// usage: memhog [chunk_kb]     (default chunk: 1024 KB)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/appstate.h"

int
main(int argc, char *argv[])
{
  int chunk_kb = 1024;
  if (argc > 1)
    chunk_kb = atoi(argv[1]);
  if (chunk_kb < 4)
    chunk_kb = 4;

  setappstate(getpid(), BACKGROUND);
  printf("memhog[%d]: started as BACKGROUND, grabbing %d KB chunks\n",
         getpid(), chunk_kb);

  int total_kb = 0;
  int failures = 0;
  for (int i = 1;; i++) {
    char *m = sbrk(chunk_kb * 1024);
    if (m == SBRK_ERROR) {
      // Out of memory before the LMK got to us; wait for it to act.
      if (++failures > 100) {
        printf("memhog[%d]: out of memory and still alive, giving up\n",
               getpid());
        exit(1);
      }
      pause(5);
      continue;
    }
    failures = 0;
    // touch every page so the memory is really used
    for (int off = 0; off < chunk_kb * 1024; off += 4096)
      m[off] = (char)i;
    total_kb += chunk_kb;

    if (i % 8 == 0)
      printf("memhog[%d]: holding %d KB\n", getpid(), total_kb);

    // give the LMK a chance to run between bursts
    if (i % 2 == 0)
      pause(1);
  }
}
