#include "command_exec.h"
#include "builtins.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define PIPE_IN 0
#define PIPE_OUT 1


void close_pipes(int pipes[][2], size_t n) {
  for (size_t i = 0; i < n; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }
}

/*             ls    |    grep .c    |    xargs cat    |    wc -l
 *
 *  NULL  <-  p[3]  <-     p[2]     <-     p[1]      <-     p[0]     <- p (Pgm linked list)
 *                 ps[2]           ps[2]             ps[0]           <- pipes array (n-1)
 *
 *          ps[2][0]      ps[1][0]        ps[0][0]        STDOUT      <- stdout redirection
 *           STDIN        ps[2][1]        ps[1][1]       ps[0][1]     <- stdin redirection
 *
 */
void exec_pipe(Pgm *p) 
{
  // count the number of commands
  size_t ncmd = 0;
  for (Pgm *t = p; t != NULL; t = t->next) ++ncmd;

  // prepare pipes
  int pipes[ncmd - 1][2]; 
  for (size_t i = 0; i < ncmd - 1; i++) {
    if (pipe(pipes[i]) < 0) {
      perror("pipe");
      exit(1);
    }
  }

  // loop through all commands
  pid_t pids[ncmd];
  Pgm* cur = p;
  for (size_t i = 0; i < ncmd; i++, cur = cur->next) {
    pid_t pid;
    if ((pid = fork()) < 0) {
      perror("fork");
      exit(1);
    }

    if (pid == 0) {
      /* *** CHILD ***
       * - connects needed pipes to stdin/-out
       *    stdout to write-end of pipe (written to by the previous command), if not last command
       *    stdout to read-end of pipe (read by the previous command)
       * - executes the command
      */

      if (i > 0) { // if not last command (not first in list)
        // connect stdout to write end of prev command (next command in list)
        dup2(pipes[i-1][1], STDOUT_FILENO); 
      }
      if (cur->next) { // if not first command (not last in list)
        // connect stdin to read end
        dup2(pipes[i][0], STDIN_FILENO);
      }

      close_pipes(pipes, ncmd - 1); // close all pipes, not needed anymore (already dup:d)
      execvp(*cur->pgmlist, cur->pgmlist);

      // if error
      perror("execvp");
      exit(1);
    }
    // parent just passes to next iteration
    pids[i] = pid;
  }

  // *** PARENT ***
  close_pipes(pipes, ncmd - 1); // close all pipes, parent doesn't use them
  for (size_t i = 0; i < ncmd; i++) {
    waitpid(pids[i], NULL, 0);
  }
}

void exec_command(Command *cmd)
{
  Pgm *p = cmd->pgm;
  if (handle_builtins(p)) return; // if builtin handled, return
  exec_pipe(p);
}

