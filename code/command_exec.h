#ifndef INCLUDE_CODE_COMMAND_EXEC_H_
#define INCLUDE_CODE_COMMAND_EXEC_H_

#include "parse.h"
#include <sys/types.h> // for pid_t
#include <sys/wait.h> // for waitpid

void exec_command(Command*);
void set_foreground_pgid(pid_t pgid); // sets the global foreground pgid variable
void clear_foreground_pgid(void); // clears the global foreground pgid variable

#endif  // INCLUDE_CODE_COMMAND_EXEC_H_

