// launcher.c: interactive terminal dashboard for the app-lifecycle OS.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/appstate.h"

#define MAXARGS 4

// ANSI escape codes for UI styling
#define CLEAR_SCREEN "\033[2J\033[H"
#define RESET        "\033[0m"
#define BOLD         "\033[1m"
#define GREEN        "\033[1;32m"
#define YELLOW       "\033[1;33m"
#define RED          "\033[1;31m"
#define CYAN         "\033[1;36m"

static void
draw_battery(void)
{
  int b = getbattery();
  int bars = b / 5; // 20 cells
  
  char *color = GREEN;
  char *icon = "🟢";

  if (b <= 20) {
    color = RED;
    icon = "🔴";
  } else if (b <= 40) {
    color = YELLOW;
    icon = "🟡";
  }

  printf("%s%s Battery: [", color, icon);
  for (int i = 0; i < 20; i++) {
    printf("%s", i < bars ? "█" : "░");
  }
  printf("] %d%%%s", b, RESET);

  if (b <= 20) {
    printf(" %s⚠️OW POWER (BG Throttled)%s", RED, RESET);
  }
  printf("\n");
}

static void
dashboard(void)
{
  printf(CLEAR_SCREEN);
  printf("%s%s========================================%s\n", CYAN, BOLD, RESET);
  printf("%s         xv6 MOBILE LAUNCHER         %s\n", BOLD, RESET);
  printf("%s%s========================================%s\n\n", CYAN, BOLD, RESET);

  draw_battery();
  
  printf("\n%s--- System Memory ---%s\n", BOLD, RESET);
  meminfo();
  
  printf("\n%s--- Active Apps ---%s", BOLD, RESET);
  procinfo();

  printf("\n%sCommands:%s\n", CYAN, BOLD);
  printf("  %srun <prog>%s   Start an app in FG\n", GREEN, RESET);
  printf("  %sfg/bg/sus%s    Change app state (e.g., fg <pid>)\n", YELLOW, RESET);
  printf("  %skill <pid>%s   Terminate an app\n", RED, RESET);
  printf("  %sbatt <pct>%s   Set battery level (0-100)\n", CYAN, RESET);
  printf("  %shog | idle%s   Spawn test workloads\n", BOLD, RESET);
  printf("  %s<enter>%s      Refresh UI     %squit%s Exit\n", BOLD, RESET, RED, RESET);
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
    printf("%slauncher: failed to start %s%s\n", RED, prog, RESET);
  else
    printf("%slauncher: started %s as pid %d%s\n", GREEN, prog, pid, RESET);
}

int
main(int argc, char *argv[])
{
  char buf[100];
  char *w[MAXARGS + 1];

  dashboard();
  for (;;) {
    printf("\n%slauncher> %s", CYAN, RESET);
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
      printf("%slauncher: idleapp pid %d (BACKGROUND)%s\n", GREEN, pid, RESET);
    } else if ((!strcmp(w[0], "fg") || !strcmp(w[0], "bg") ||
                !strcmp(w[0], "sus")) && n >= 2) {
      int st = w[0][0] == 'f' ? FOREGROUND
             : w[0][0] == 'b' ? BACKGROUND : SUSPENDED;
      if (setappstate(atoi(w[1]), st) < 0)
        printf("%slauncher: no such pid %s%s\n", RED, w[1], RESET);
    } else if (!strcmp(w[0], "kill") && n >= 2) {
      if (kill(atoi(w[1])) < 0)
        printf("%slauncher: no such pid %s%s\n", RED, w[1], RESET);
      pause(3);
    } else if (!strcmp(w[0], "batt") && n >= 2) {
      if (setbattery(atoi(w[1])) < 0)
        printf("%slauncher: battery must be 0-100%s\n", RED, RESET);
    } else {
      printf("%slauncher: unknown command%s\n", RED, RESET);
      pause(10);
    }
    pause(2);
    dashboard();
  }
  printf("launcher: bye\n");
  exit(0);
}
