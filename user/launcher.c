// launcher: interactive terminal dashboard for the app-lifecycle OS.
//
// Shows battery, free memory and the process table, and lets you start
// apps and move them between FOREGROUND / BACKGROUND / SUSPENDED.
//
// Apps are started with a double fork so they are re-parented to init,
// which reaps them when they exit or are killed by the LMK. (If the
// launcher were the parent, killed apps would stay zombies holding their
// memory while the launcher sits blocked waiting for input.)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/appstate.h"

#define MAXARGS 4

static void
draw_battery(void)
{
  int b = getbattery();
  int bars = b / 5; // 20 cells
  printf("Battery: [");
  for (int i = 0; i < 20; i++)
    printf("%c", i < bars ? '#' : '.');
  printf("] %d%%", b);
  if (b < 20)
    printf("  (LOW - background apps throttled)");
  printf("\n");
}

static void
dashboard(void)
{
  printf("\033[2J\033[H");
  printf("=========== xv6 APP LAUNCHER ===========\n");
  draw_battery();
  meminfo();
  procinfo();
  printf("\nCommands:\n");
  printf("  run <prog> [args]  start an app (FOREGROUND)\n");
  printf("  hog | idle         start memhog / idleapp\n");
  printf("  fg|bg|sus <pid>    change app state\n");
  printf("  kill <pid>         kill an app\n");
  printf("  batt <0-100>       set battery level\n");
  printf("  <enter>            refresh      quit  exit\n");
}

// start prog as an orphan; returns its pid or -1
static int
launch(char *prog, char **argv)
{
  int fds[2];
  int pid = -1;

  if (pipe(fds) < 0)
    return -1;
  int a = fork();
  if (a < 0) {
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
  if (a == 0) {
    int b = fork();
    if (b == 0) {
      close(fds[0]);
      close(fds[1]);
      exec(prog, argv);
      printf("launcher: cannot exec %s\n", prog);
      exit(1);
    }
    write(fds[1], &b, sizeof(b));
    exit(0); // child b becomes an orphan, adopted by init
  }
  close(fds[1]);
  if (read(fds[0], &pid, sizeof(pid)) != sizeof(pid))
    pid = -1;
  close(fds[0]);
  wait(0); // reap the short-lived intermediate process
  return pid;
}

// split line into words; returns count
static int
split(char *line, char **words, int max)
{
  int n = 0;
  char *p = line;
  while (*p && n < max) {
    while (*p == ' ' || *p == '\t' || *p == '\n')
      *p++ = 0;
    if (!*p)
      break;
    words[n++] = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n')
      p++;
  }
  while (*p == ' ' || *p == '\t' || *p == '\n')
    *p++ = 0;
  return n;
}

static void
start_app(char *prog, char **argv)
{
  int pid = launch(prog, argv);
  if (pid < 0)
    printf("launcher: failed to start %s\n", prog);
  else
    printf("launcher: started %s as pid %d\n", prog, pid);
}

int
main(int argc, char *argv[])
{
  char buf[100];
  char *w[MAXARGS + 1];

  dashboard();
  for (;;) {
    printf("\nlauncher> ");
    memset(buf, 0, sizeof(buf));
    gets(buf, sizeof(buf));
    if (buf[0] == 0)
      break; // EOF
    int n = split(buf, w, MAXARGS);

    if (n == 0) {
      // just refresh
    } else if (!strcmp(w[0], "quit") || !strcmp(w[0], "q")) {
      break;
    } else if (!strcmp(w[0], "run") && n >= 2) {
      w[n] = 0;
      start_app(w[1], &w[1]);
      pause(5);
    } else if (!strcmp(w[0], "hog")) {
      char *a[] = {"memhog", 0};
      start_app("memhog", a);
      pause(5);
    } else if (!strcmp(w[0], "idle")) {
      char *a[] = {"idleapp", 0};
      int pid = launch("idleapp", a);
      if (pid > 0)
        setappstate(pid, BACKGROUND);
      printf("launcher: idleapp pid %d (BACKGROUND)\n", pid);
    } else if ((!strcmp(w[0], "fg") || !strcmp(w[0], "bg") ||
                !strcmp(w[0], "sus")) && n >= 2) {
      int st = w[0][0] == 'f' ? FOREGROUND
             : w[0][0] == 'b' ? BACKGROUND : SUSPENDED;
      if (setappstate(atoi(w[1]), st) < 0)
        printf("launcher: no such pid %s\n", w[1]);
    } else if (!strcmp(w[0], "kill") && n >= 2) {
      if (kill(atoi(w[1])) < 0)
        printf("launcher: no such pid %s\n", w[1]);
      pause(3);
    } else if (!strcmp(w[0], "batt") && n >= 2) {
      if (setbattery(atoi(w[1])) < 0)
        printf("launcher: battery must be 0-100\n");
    } else {
      printf("launcher: unknown command\n");
      pause(10);
    }
    pause(2);
    dashboard();
  }
  printf("launcher: bye\n");
  exit(0);
}
