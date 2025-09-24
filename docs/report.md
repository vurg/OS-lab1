# **lsh Report**

**Authors:** Muhab Jabraouti, Nasit Vurgun, Maksims Orlovs
**Course:**  EDA093
**Date:**  25-09-2025

## **1. Introduction**

This report documents the development lifecycle of `lsh`, including how the lab specifications were met, the order of 
implementation, challenges encountered, as well as feedback on the laboratory materials and testing procedures.

## **2. Order of fulfillment of Specifications**

We successfully implemented **all specifications** for the `lsh` shell. The following is a detailed account of how 
each requirement was met, in the order of implementation.

**2.1. Ctrl-D Handling:** The shell gracefully exits upon receiving an End-of-File (EOF) signal. This was achieved by 
checking the return value of the `readline()` library function; a `NULL` return value indicates EOF, which triggers a clean exit from the main command loop.

**2.2. Basic Commands with PATH:** The shell correctly executes simple commands like `ls` and `date`. This 
functionality is driven by the `execvp()` system call, which automatically searches for the executable in the 
directories listed in the `PATH` environment variable. If a command is not found, `execvp()` fails, and an appropriate 
error message is printed to the user.

**2.3. Built-in Commands:** The `cd` and `exit` commands are implemented as built-ins, meaning they are executed 
directly within the shell process itself without forking a child. The `cd` command uses the `chdir()` system call, and 
the `exit` command breaks the main loop, leading to a graceful termination.

**2.4. Piping:** Piping is handled by creating the needed pipes before iterating the pipeline using `pipe`, and 
redirecting the IO as needed at each pipeline stage using `dup2`.

**2.5. Background Execution:** Commands terminated with an ampersand (`&`) are executed in the background. The shell 
identifies the `background` flag in the parsed `Command` structure and, after forking the child process, does not call 
`waitpid()`, and instead reaps the background children asynchronously, by ignoring `SIGCHLD`, which automatically 
terminates the child process.

**2.6. I/O Redirection:** The shell handles two forms of redirection: stdin (`<`), stdout(`>`). This is implemented 
using `open()` and `dup2()` to reassign standard input of the first command (`STDIN_FILENO`) or standard output of the 
last command (`STDOUT_FILENO`) to the specified files before calling `execvp()`.

**2.7. Ctrl-C Handling:** The shell handles Ctrl-C (`SIGINT`) using process groups and terminal foreground. Each 
pipeline creates its own process group with the rightmost command as leader, and all following commands are assigned 
to this process group using `setpgid` system call. Then, after all the children are created, the shell sets the 
pipeline as terminal foreground process group, and re-sets the shell as foreground after the pipeline is done using `tcsetpgrp`.

**2.8. No Zombies:** During implementation of previous requirements (especially 2, 4, and 5), and after everything was 
implemented, we ensured that no zombies are created in any scenario, using the provided unit tests, manual tests from 
`README.md`, as well as ad-hoc testing. The main mechanism used to ensure that no zombies are created is reaping the 
children by: a. waiting for each process spawned by a pipeline (in case of foreground execution); b. ignoring 
`SIGCHLD` to asynchronously terminate child processes that were executing in the background upon their exit.

## **3. Challenges Encountered**

### **3.1. Pipeline Handling with reverse-order Parsing**

The parser provided in the skeleton returned a linked list of pipeline commands in reversed order. This added 
complexity to implementing piping, and caused us to attempt 3 different implementations, before settling on a final one.

- **The first implementation** was the simplest (and perhaps naive). We first reversed the linked list, then iterated 
through it (see #5). The plan was to later add piping similar to the final solution. This solution was quickly 
rejected because the reversal of the list felt inefficient and unnecessary, and the team decided to look for a more elegant solution.
- **The second implementation** used recursion for traversing the linked list and executing commands. A “pipeline 
root” process was forked off the shell, then that process would `fork` recursively: the parent would execute the right 
side of the pipe (rightmost command), and the child would execute the rest of the pipeline (remainder to the left of 
the pipe), by recursing further into the list (see #6).
We decided against this solution due to the resulting parent-child structure being a tree (each command is child of 
next command), instead of flat like in real shells (each command is child of shell). This raised concerns about zombie 
processes - the shell only waited for the pipeline root process (last command). While it seemed that the kernel 
automatically cleaned up the child processes, the team decided to change the implementation so that the shell 
explicitly reaps all children to avoid any possibility of zombies. Another concern that caused dismissal of this 
solution was the problems related to recursion when running long pipelines, such as potential stack overflows, memory 
inefficiency, and difficulties with process management.
- **The final implementation** is iterative and allowed for explicit process management, while also keeping the list 
order. The algorithm sets up all pipes beforehand, then iterates through the list as-is (in reverse order), does all 
the needed redirections, and then executes each command in a forked process (see #9).

### **3.2. Foreground Process Group and Unit Test failure**

To implement `SIGINT` handling, we decided to put every step of a pipeline into a single pgroup. Then, the shell could 
keep track of the active foreground process, and explicitly redirect `SIGINT` to it with `kill` syscall (or ignore if 
there is no foreground process). While this implementation made the relevant tests pass, we noticed during ad-hoc 
testing that if a foreground process needs STDIN/-OUT (e.g. running `cat` with no parameters), the shell would create 
a process with status “T” that would never be terminated, and the shell would become unresponsive if attempting to 
terminate this process with CTRL-C. This prompted us to research further, and find the `tsetpgrp` syscall, which 
allowed us to set the pipeline as a foreground process in the terminal while it executes. While this solution is more 
complete and allows to run commands that need STDIN, the associated unit tests fail because they send a `SIGINT` to 
the shell, not knowing that it is in the background and always ignores `SIGINT`:s, because the foreground process 
would get the `SIGINT` under normal conditions.

## **4. Feedback on Lab Materials**

### **4.1. Repository**

The repository structure and the provided skeleton were clear and easy to work with, and allowed to focus on the 
actual shell implementation and not worry about parsing.

### **4.2. Automated tests**

The tests were useful in determining if the *basic* solution was implemented correctly, but the test set felt too 
shallow to allow us to bypass manual testing and rely only on the CI pipeline. In some cases we discovered problems 
with implementation even if the relevant test was green. Here are some problems we saw:

- `test_bg_and_fg` passed even before backgrounding was implemented, making it unclear if the implementation was 
actually working and *requiring* manual testing.
- `test_CTRL_C*` tests failed with pgroup implementation (see section 3.2)

