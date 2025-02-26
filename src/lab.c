/**
 * simple_shell.c
 * A basic shell implementation featuring command parsing, built-in commands, and process execution.
 * 
 */

 #include "simple_shell.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <readline/readline.h>
 #include <readline/history.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <pwd.h>
 #include <errno.h>
 #include <signal.h>
 
 #define MAX_ARGS sysconf(_SC_ARG_MAX)
 
 /**
  * @brief Processes command-line arguments for shell initialization.
  * Detects the '-v' flag to display version info and exit.
  *
  * @param argc Number of command-line arguments.
  * @param argv Argument vector.
  */
 void handle_args(int argc, char **argv) {
     int option;
     while ((option = getopt(argc, argv, "v")) != -1) {
         if (option == 'v') {
             printf("Shell Release: %d.%d\n", shell_VERSION_MAJOR, shell_VERSION_MINOR);
             exit(0);
         }
     }
 }
 
 /**
  * @brief Retrieves the shell prompt from an environment variable.
  * Defaults to "myshell> " if unset.
  *
  * @param env Variable storing the prompt.
  * @return A dynamically allocated prompt string (must be freed by caller).
  */
 char *fetch_prompt(const char *env) {
     char *prompt = getenv(env);
     if (!prompt) {
         prompt = "myshell> ";
     }
     return strdup(prompt);
 }
 
 /**
  * @brief Tokenizes an input string into an array of arguments.
  *
  * @param input The command string entered by the user.
  * @return A dynamically allocated array of arguments (must be freed later).
  */
 char **split_command(const char *input) {
     char **args = malloc(MAX_ARGS * sizeof(char *));
     if (!args) return NULL;
 
     char *token, *input_copy = strdup(input);
     int index = 0;
 
     token = strtok(input_copy, " ");
     while (token && index < MAX_ARGS - 1) {
         args[index++] = strdup(token);
         token = strtok(NULL, " ");
     }
     args[index] = NULL;
 
     free(input_copy);
     return args;
 }
 
 /**
  * @brief Releases memory allocated for the parsed command.
  *
  * @param args Command argument list.
  */
 void free_command(char **args) {
     if (!args) return;
     for (int i = 0; args[i] != NULL; i++) {
         free(args[i]);
     }
     free(args);
 }
 
 /**
  * @brief Strips leading and trailing whitespace from a string.
  *
  * @param input The string to be trimmed.
  * @return Pointer to the modified string.
  */
 char *strip_whitespace(char *input) {
     while (isspace((unsigned char)*input)) input++;
     if (*input == 0) return input;
     char *end = input + strlen(input) - 1;
     while (end > input && isspace((unsigned char)*end)) end--;
     end[1] = '\0';
     return input;
 }
 
 /**
  * @brief Handles changing directories in the shell.
  * Defaults to the home directory if no path is provided.
  *
  * @param args Argument array (expects "cd [directory]").
  * @return 0 on success, -1 on failure.
  */
 int handle_cd(char **args) {
     if (!args[1]) {
         const char *home_dir = getenv("HOME");
         if (!home_dir) {
             struct passwd *pw = getpwuid(getuid());
             home_dir = pw ? pw->pw_dir : NULL;
         }
         if (home_dir && chdir(home_dir) != 0) {
             perror("cd");
             return -1;
         }
     } else if (chdir(args[1]) != 0) {
         perror("cd");
         return -1;
     }
     return 0;
 }
 
 /**
  * @brief Executes built-in commands such as exit, cd, and history.
  *
  * @param sh Shell instance.
  * @param argv Parsed command arguments.
  * @return True if the command was built-in and executed, false otherwise.
  */
 bool execute_builtin(struct shell *sh, char **argv) {
     if (!argv[0]) return false;
 
     if (strcmp(argv[0], "exit") == 0) {
         cleanup_shell(sh);
         exit(0);
     } else if (strcmp(argv[0], "cd") == 0) {
         return handle_cd(argv) == 0;
     } else if (strcmp(argv[0], "history") == 0) {
         HIST_ENTRY **hist_entries = history_list();
         if (hist_entries) {
             for (int i = 0; hist_entries[i]; i++) {
                 printf("%d  %s\n", i + history_base, hist_entries[i]->line);
             }
         }
         return true;
     }
     return false;
 }
 
 /**
  * @brief Sets up the shell environment and signal handling.
  *
  * @param sh Shell instance.
  */
 void setup_shell(struct shell *sh) {
     sh->shell_terminal = STDIN_FILENO;
     sh->shell_is_interactive = isatty(sh->shell_terminal);
 
     if (sh->shell_is_interactive) {
         while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
             kill(-sh->shell_pgid, SIGTTIN);
 
         signal(SIGINT, SIG_IGN);
         signal(SIGQUIT, SIG_IGN);
         signal(SIGTSTP, SIG_IGN);
         signal(SIGTTIN, SIG_IGN);
         signal(SIGTTOU, SIG_IGN);
 
         sh->shell_pgid = getpid();
         setpgid(sh->shell_pgid, sh->shell_pgid);
         tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
 
         tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
     }
 
     sh->prompt = fetch_prompt("CUSTOM_PROMPT");
 }
 
 /**
  * @brief Cleans up shell resources before termination.
  *
  * @param sh Shell instance.
  */
 void cleanup_shell(struct shell *sh) {
     free(sh->prompt);
 }
 
 /**
  * @brief Runs an external command using fork and execvp.
  *
  * @param args Parsed command arguments.
  */
 void run_command(char **args) {
     if (!args || !args[0]) return;
 
     pid_t child_pid = fork();
     if (child_pid == 0) {
         signal(SIGINT, SIG_DFL);
         signal(SIGQUIT, SIG_DFL);
         signal(SIGTSTP, SIG_DFL);
         signal(SIGTTIN, SIG_DFL);
         signal(SIGTTOU, SIG_DFL);
 
         execvp(args[0], args);
         perror("execution failed");
         exit(EXIT_FAILURE);
     } else if (child_pid > 0) {
         int status;
         waitpid(child_pid, &status, 0);
     } else {
         perror("fork error");
     }
 }
 