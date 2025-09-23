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
#include <ctype.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "parse.h"
#include "command_exec.h"

// *** PROCESS GROUP MANAGEMENT ***
// we want the shell to terminate the entire pipeline, not just one process
// without process groups, signals would only affect individual processes
// process groups allow you to send signals to all processes in the pipeline at once


// *** SIGNAL HANDLER ***
// Handler for SIGINT (Ctrl-C)
//  if the shell is in foreground, do not stop it
static void sigint_handler(int sig) {
    // If there is a foreground process group, terminal control is passed to it
    // therefore SIGINT will go to that process group, not this handler
    printf("\n");
    rl_on_new_line(); // Inform readline that the cursor is on a new line
    rl_replace_line("", 0); // Clear the current input line
    rl_redisplay(); // Redisplay the shell prompt
}

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);

int main(void) {
  // *** SETUP ***
  // parse the "DEBUG" env variable, define DEBUG if it was present and not 0
  char *DEBUG_STR = getenv("DEBUG");
  int DEBUG = 0;
  if (DEBUG_STR) {
    int conv = atoi(DEBUG_STR);
    if (conv != 0) {
      DEBUG = 1;
    }
  }

  // ignore SIGHCLD:s, which makes exited children be removed instead of turned into zombies
  signal(SIGCHLD, SIG_IGN);
  
  // ignore background read/write signals to prevent shell from being terminated
  // if doing i/o while in background pgroup (while pipeline is in foreground)
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  // handle SIGINT, do not terminate the shell, only the foreground process group
  signal(SIGINT, sigint_handler);

  // *** MAIN LOOP ***
  char path_buff[PATH_MAX];
  for (;;)
  {
    char *line;
    char *prompt;
    if (getcwd(path_buff, PATH_MAX) != NULL) {
      size_t prompt_len = strlen(path_buff) + 2;
      prompt = malloc((prompt_len + 1) * sizeof(char));
      snprintf(prompt, prompt_len + 1, "%s> ", path_buff);
    }

    line = readline(prompt);
    free(prompt);

    if (rl_eof_found) {
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
        exec_command(&cmd);

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

