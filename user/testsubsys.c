#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("--- Testing meminfo ---\n");
  meminfo();

  printf("\n--- Testing procinfo ---\n");
  procinfo();

  printf("\n--- Testing battery syscalls ---\n");
  int b = getbattery();
  printf("Current battery: %d%%\n", b);

  printf("Setting battery to 15%%...\n");
  setbattery(15);
  printf("Updated battery: %d%%\n", getbattery());

  // Reset back to 100%
  setbattery(100);

  exit(0);
}
