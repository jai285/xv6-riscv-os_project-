#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/appstate.h"

void
worker(void)
{
  volatile long long count = 0;
  for (long long i = 0; i < 600000000LL; i++) {
    count += i;
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("--- Low-Battery Throttling Test ---\n");
  setpolicy(1);

  setbattery(100);
  printf("\n[Phase 1] Battery = 100%%. Spawning FG and BG workers...\n");

  int fg1 = fork();
  if (fg1 == 0) worker();
  setappstate(fg1, FOREGROUND);

  int bg1 = fork();
  if (bg1 == 0) worker();
  setappstate(bg1, BACKGROUND);

  pause(50);
  printf("\n--- Mid-run at 100%% Battery ---");
  procinfo();

  wait(0);
  wait(0);

  setbattery(10);
  printf("\n[Phase 2] Battery = 10%%. Spawning FG and BG workers...\n");

  int fg2 = fork();
  if (fg2 == 0) worker();
  setappstate(fg2, FOREGROUND);

  int bg2 = fork();
  if (bg2 == 0) worker();
  setappstate(bg2, BACKGROUND);

  pause(50);
  printf("\n--- Mid-run at 10%% Battery (BG should have far fewer ticks) ---");
  procinfo();

  wait(0);
  wait(0);

  // Restore battery
  setbattery(100);
  printf("\nTest finished. Battery restored to 100%%.\n");
  exit(0);
}
