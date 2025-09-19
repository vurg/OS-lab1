#include "builtins.h"

#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void cd_builtin(const char* arg) {
  if (!arg)
  {
    fprintf(stderr, "cd: missing argument\n");
  }
  else if (chdir(arg) != 0)
  {
    perror("cd");
  }
}


int handle_builtins(Pgm* p) {
  char* cmd = p->pgmlist[0];
  if (strcmp(cmd, "exit") == 0)
  {
    exit(0);
    return 1;
  }
  else if (strcmp(cmd, "cd") == 0)
  {
    cd_builtin(p->pgmlist[1]);
    return 1;
  }
  else if (strcmp(cmd, "..") == 0) {
    cd_builtin("..");
    return 1;
  }

  return 0; // not a builtin
}
