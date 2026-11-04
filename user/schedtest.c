#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/appstate.h"

void
spin(void)
{
  volatile long long count = 0;
  for (int i = 0; i < 500000000LL; i++) {
    count += i;
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("--- Schedtest: Starting Priority & State Test ---\n");

  // 1. Fork Foreground Worker
  int pid_fg = fork();
  if (pid_fg == 0) {
    spin();
  }
  setappstate(pid_fg, FOREGROUND);

  // 2. Fork Background Worker
  int pid_bg = fork();
  if (pid_bg == 0) {
    spin();
  }
  setappstate(pid_bg, BACKGROUND);

  // 3. Fork Suspended Worker
  int pid_su = fork();
  if (pid_su == 0) {
    spin();
  }
  setappstate(pid_su, SUSPENDED);

  printf("Spawned tasks: FG (PID %d), BG (PID %d), SU (PID %d)\n", pid_fg, pid_bg, pid_su);

  // Allow tasks to run for several timer ticks
  pause(5);

  printf("\n--- Mid-Run Process Status ---");
  procinfo();

  // Resume suspended task so it can finish
  printf("\nResuming SUSPENDED process %d to BACKGROUND...\n", pid_su);
  setappstate(pid_su, BACKGROUND);

  // Wait for all child processes to terminate
  wait(0);
  wait(0);
  wait(0);

  printf("\n--- Final Process Status ---");
  procinfo();

  exit(0);
}

