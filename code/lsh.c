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

#include "parse.h"
#include <sys/types.h>

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);

Pgm *reverse_list(Pgm *head)
{
  Pgm *prev = NULL;
  Pgm *current = head;
  Pgm *next = NULL;

  while (current != NULL)
  {
    next = current->next; // Store next node
    current->next = prev; // Reverse the link
    prev = current;       // Move prev to this node
    current = next;       // Move to next node
  }

  return prev; // New head of the reversed list
}


void run_command(Command *cmd_list)
{
  Pgm *pl = cmd_list->pgm;
  int length = 0;

  // Count number of commands
  Pgm *tmp = pl;
  while (tmp != NULL)
  {
    length++;
    tmp = tmp->next;
  }

  // Reverse the list for left-to-right execution
  pl = reverse_list(pl);

  // Run commands
  for (int i = 0; i < length; i++)
  {

    // Built-in: exit
    if (strcmp(pl->pgmlist[0], "exit") == 0)
    {
      exit(0);
    }
    // Built-in: cd
    else if (strcmp(pl->pgmlist[0], "cd") == 0)
    {
      if (pl->pgmlist[1] == NULL)
      {
        fprintf(stderr, "cd: missing argument\n");
      }
      else if (chdir(pl->pgmlist[1]) != 0)
      {
        perror("cd failed");
      }

      pl = pl->next;
      continue;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
      printf(" Command: %s \n", pl->pgmlist[0]);
      execvp(pl->pgmlist[0], pl->pgmlist);
      perror("execvp failed");
      exit(1);
    }
    else if (pid > 0)
    {
      int status;
      waitpid(pid, &status, 0);
    }
    else
    {
      perror("fork failed");
      exit(1);
    }

    pl = pl->next;
  }
}

int main(void)
{
  for (;;)
  {
    char *line;
    line = readline("> ");

    if (line == NULL)
    {
      break;
    }

    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If stripped line not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1)
      {
        // Just prints cmd
        print_cmd(&cmd);
        run_command(&cmd);
      }
      else
      {
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
