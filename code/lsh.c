/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file(s)
 * you will need to modify the CMakeLists.txt to compile
 * your additional file(s).
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Using assert statements in your code is a great way to catch errors early and make debugging easier.
 * Think of them as mini self-checks that ensure your program behaves as expected.
 * By setting up these guardrails, you're creating a more robust and maintainable solution.
 * So go ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "parse.h"

#define PIPE_IN 0
#define PIPE_OUT 1

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);

void execute_pgms(Pgm* p);

int main(void) {
  char *DEBUG_STR = getenv("DEBUG");
  int DEBUG = 0;
  if (DEBUG_STR) {
    int conv = atoi(DEBUG_STR);
    if (conv != 0) DEBUG = 1;
  }

  for (;;)
  {
    char *line;
    line = readline("> ");

    if (line == NULL) {
      break;
    }

    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If stripped line not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1) {
        execute_pgms(cmd.pgm);

        // Just prints cmd
        if (DEBUG) print_cmd(&cmd);
      }
      else {
        printf("Parse ERROR\n");
      }
    }

    // Clear memory
    free(line);
  }

  return 0;
}

void exec_recursive(Pgm *p) {
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

    exec_recursive(p->next); // recurse further left
  }
}

void execute_pgms(Pgm *p) {
  // fork for the entire pipeline
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }

  if (pid == 0) {
    // child executes the pipeline recursively
    exec_recursive(p);
    exit(1); // sanity check
  } else {
    // parent waits for the whole pipeline
    waitpid(pid, NULL, 0);
  }
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
static void print_cmd(Command *cmd_list)
{
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
  printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
  printf("background: %s\n", cmd_list->background ? "true" : "false");
  printf("Pgms:\n");
  print_pgm(cmd_list->pgm);
  printf("------------------------------\n");
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
{
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  size_t i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}

