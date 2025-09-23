#include "command_exec.h"
#include "builtins.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define PIPE_IN 0
#define PIPE_OUT 1

// External functions from lsh.c to manage foreground process group ID
extern void set_foreground_pgid(pid_t pgid);
extern void clear_foreground_pgid(void);

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
void exec_pipe(Pgm *p, int redir_fd[2], int is_background)
{
  pid_t pgid = 0; // process group ID of the pipeline
  
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

  if (is_background) printf("[bg]"); // header for printing out pid:s

  // loop through all commands
  pid_t pids[ncmd]; // array to store each command's pid
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

      // Always create new process group for the first child
      if (i == 0) {
        setpgid(0, 0); // create new process group with this process as leader
      } else {
        // parent sets up process group for other children - parent calls setpgid
      }

      if (i > 0) { // if not last command (not first in list)
        // connect stdout to write end of prev command (next command in list)
        dup2(pipes[i-1][1], STDOUT_FILENO); 
      } else if (redir_fd[1] != -1 && redir_fd[1] != STDOUT_FILENO){
        dup2(redir_fd[1], STDOUT_FILENO);
      }
      if (cur->next) { // if not first command (not last in list)
        // connect stdin to read end
        dup2(pipes[i][0], STDIN_FILENO);
      } else if (redir_fd[0] != -1 && redir_fd[0] != STDIN_FILENO) {
        dup2(redir_fd[0], STDIN_FILENO);
      }

      close_pipes(pipes, ncmd - 1); // close all pipes, not needed anymore (already dup:d)
      execvp(*cur->pgmlist, cur->pgmlist);

      // if error
      perror("execvp");
      exit(1);

    } else {

      /* *** PARENT *** */
      pids[i] = pid;
      
      // Set process group for ALL children from parent side as well
      // so that we don't rely on child executing first:
      // no matter which process runs first, the pgid will be set correctly
      if (i == 0) {
        pgid = pid; // first child becomes the process group leader
        setpgid(pid, 0); // create new process group with first child as leader
      } else {
        setpgid(pid, pgid); // add subsequent children to the group
      }
      
      if (is_background) 
        printf("\t%d %s %s\n", pid, *cur->pgmlist, (cur->next) ? "|" : ""); // print out pid:s and associated commands
    }
  }

  close_pipes(pipes, ncmd - 1); // close all pipes, parent doesn't use them

  // Set foreground process group if not background
  if (!is_background) {
    set_foreground_pgid(pgid);
  }

  // wait for each foreground process (bg processes reaped asynchronously)
  if (!is_background) {
    for (size_t i = 0; i < ncmd; i++) {
      waitpid(pids[i], NULL, 0);
    }
    clear_foreground_pgid(); // clear the foreground pgid variable
  }
}

void exec_command(Command *cmd)
{
  Pgm *p = cmd->pgm;
  if (handle_builtins(p)) return; // if builtin handled, return

  int redir_fd[2] = {-1, -1};
  if (cmd->rstdin) {
    if ((redir_fd[0] = open(cmd->rstdin, O_RDONLY)) == -1) {
      perror("open");
      return;
    }
  }
  if (cmd->rstdout) {
    if ((redir_fd[1] = open(cmd->rstdout, O_WRONLY | O_APPEND | O_CREAT, 0644)) == -1) {
      perror("open");
      return;
    }
  }

  exec_pipe(p, redir_fd, cmd->background);

  for (int i = 0; i < 2; i++) {
    if (redir_fd[i] != -1)
      close(redir_fd[i]);
  }
}

