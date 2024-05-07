#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define ARR_SIZE 1<<16

extern char **environ;

//Store the environment variable pair
typedef struct Shell_environ{
	char *variable;
	char *variable_value;
} Shell_environ;

//Keep track of how many shell variable currently

void extract_replace_env_variable(Shell_environ *shell_environ, int num_shell_environ,char **str);
void get_shell_environ(Shell_environ *shell_environ, int num_shell_environ, char *variable, char *result);
void set_shell_variable(Shell_environ *shell_environ, int *num_shell_environ, char *variable, char *variable_value);
char set_environment_variable(Shell_environ *shell_environ, int *shell_environ_number, char **args, int nargs);

#endif