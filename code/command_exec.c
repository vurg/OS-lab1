#include "command_exec.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define PIPE_IN 0
#define PIPE_OUT 1


void exec_command(Command *cmd) {
  // fork for the entire pipeline
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }

  if (pid == 0) {
    // child executes the pipeline recursively
    exec_pipe(cmd->pgm);
    exit(1); // sanity check
  } else {
    // parent waits for the whole pipeline
    waitpid(pid, NULL, 0);
  }
}

void exec_pipe(Pgm *p) {
  /* base case - leftmost command => execute and exit */
  if (p->next == NULL) {
    execvp(p->pgmlist[0], p->pgmlist);
    perror("execvp");
    exit(1);
  }

  /* create a pipe that will be shared by recursive calls through fork() */
  int pipefd[2];
  if (pipe(pipefd) < 0) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid > 0) {
    /* *** PARENT ***
     * executes the right-hand side of a given pipe (one command)
     * input will come from the pipe, provided by the previous recursive call
     * (the pipe is shared between calls since fork() create a copy of fd table)
     */

    close(pipefd[PIPE_OUT]); // not needed, parent doesnt write to pipe

    dup2(pipefd[PIPE_IN], STDIN_FILENO); // stdin comes from child’s output
    close(pipefd[PIPE_IN]);

    execvp(p->pgmlist[0], p->pgmlist);
    perror("execvp");
    exit(1);
  } else {
    /* *** CHILD *** 
     * executes the left-hand side of a given pipe (rest of pipeline)
     * by redirecting stdout to a pipe and recursing
     * further left into the pipeline (builds pipeline right-to-left)
     */

    close(pipefd[PIPE_IN]); // not needed, child doesnt read from pipe

    dup2(pipefd[PIPE_OUT], STDOUT_FILENO); // redirect stdout to pipe write end
    close(pipefd[PIPE_OUT]); // not needed anymore (already dup:ed)

    exec_pipe(p->next); // recurse further left
  }
}
