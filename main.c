/* 3000shell.c */
/* v2 Sept. 15, 2019 */
/* v1 Sept. 24, 2017 */
/* based off of csimpleshell.c, Enrico Franchi © 2005
      https://web.archive.org/web/20170223203852/
      http://rik0.altervista.org/snippets/csimpleshell.html */
/* Original under "BSD" license */
/* This version is under GPLv3, copyright 2017, 2019 Anil Somayaji */
/* You really shouldn't be incorporating parts of this in any other code,
   it is meant for teaching, not production */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 1<<16
#define ARR_SIZE 1<<16
#define COMM_SIZE 32

const char *proc_prefix = "/proc";

extern char **environ;

//Store the environment variable pair
struct Shell_environ{
	char *variable;
	char *variable_value;
}shell_environ[ARR_SIZE];

//Keep track of how many shell variable currently

int shell_environ_number = 0;

void get_shell_environ(char *variable, char **result){
	for(int i = 0; i < shell_environ_number; ++i){
		if(!strcmp(variable, shell_environ[i].variable)){
			*result = shell_environ[i].variable_value;
			break;
		}		
	}
}
void extract_replace_env_variable(char **str){
	char result[ARR_SIZE];
	int strSize = strlen(*str);

	char *it = *str;
	
	char *current = result;
	while(*it != '\0'){
		//If it is not $ sign, keep copying
		if(*it != '$'){
			*current = *it;
			current++;
			it++;
		}else{
			//Skip the dolar sign
			it++;
			char variable[ARR_SIZE];
			char *cp = variable;
			//Get the variable name until meet the separator or another $ sign
			while(*it != '\0' && *it != '/' && *it != '.' && *it != '$'){
				*cp = *it;
				it++;
				cp++;
			}
			*cp = '\0';
			if(getenv(variable) != NULL){
				strcpy(current, getenv(variable));
				current += strlen(getenv(variable));
			}else{
				//check if it exist in shell variable
				char *variable_value = NULL;
				get_shell_environ(variable, &variable_value);
				if(variable_value != NULL){
					strcpy(current, variable_value);
					current += strlen(variable_value);
					continue;
				}
				//If it still not exist, then just act as if it is normal string 
				*current = '$';
				current++;
				strcpy(current, variable);
				current += strlen(variable);
			}
		}
	}
	*str = result;
}

void parse_args(char *buffer, char** args, 
                size_t args_size, size_t *nargs)
{
        char *buf_args[args_size]; /* You need C99 */
        char **cp, *wbuf;
        size_t i, j;
        
        wbuf=buffer;
        buf_args[0]=buffer; 
        args[0] =buffer;
        
        for(cp=buf_args; (*cp=strsep(&wbuf, " \n\t")) != NULL ;){
                if ((*cp != NULL) && (++cp >= &buf_args[args_size]))
                        break;
        }
        
        for (j=i=0; buf_args[i]!=NULL; i++){
                if (strlen(buf_args[i]) > 0){
			if(strchr(buf_args[i], '$') != NULL){
				extract_replace_env_variable(&buf_args[i]);
			}
                        args[j++]=buf_args[i];
		}
        }
        
        *nargs=j;
        args[j]=NULL;

}

/* this is kind of like getenv() */
char *find_env(char *envvar, char *notfound, char *envp[])
{
        const int MAXPATTERN = 128;
        int i, p;
        char c;
        char pattern[MAXPATTERN];
        char *value = NULL;

        p = 0;
        while ((c = envvar[p])) {
                pattern[p] = c;
                p++;
                if (p == (MAXPATTERN - 2)) {
                        break;
                }
        }
        pattern[p] = '=';
        p++;
        pattern[p] = '\0';
        
        i = 0;
        while (envp[i] != NULL) {
                if (strncmp(pattern, envp[i], p) == 0) {                        
                        value = envp[i] + p;
                }
                i++;
        }

        if (value == NULL) {
                return notfound;
        } else {
                return value;
        }
}

void find_binary(char *name, char *path, char *fn, int fn_size) {
        char *n, *p;
        int r, stat_return;

        struct stat file_status;

        if (name[0] == '.' || name[0] == '/') {
                strncpy(fn, name, fn_size);
                return;
        }
        
        p = path;
        while (*p != '\0') {       
                r = 0;
                while (*p != '\0' && *p != ':' && r < fn_size - 1) {
                        fn[r] = *p;
                        r++;
                        p++;
                }

                fn[r] = '/';
                r++;
                
                n = name;
                while (*n != '\0' && r < fn_size) {
                        fn[r] = *n;
                        n++;
                        r++;
                }
                fn[r] = '\0';

                
                stat_return = stat(fn, &file_status);

                if (stat_return == 0) {
                        return;
                }

                if (*p != '\0') {
                        p++;
                }
        }
}

void setup_comm_fn(char *pidstr, char *comm_fn)
{
        char *c;

        strcpy(comm_fn, proc_prefix);
        c = comm_fn + strlen(comm_fn);
        *c = '/';
        c++;
        strcpy(c, pidstr);
        c = c + strlen(pidstr);
        strcpy(c, "/comm");
}

void plist()
{
        DIR *proc;
        struct dirent *e;
        int result;
        char comm[COMM_SIZE];  /* seems to just need 16 */        
        char comm_fn[512];
        int fd, i, n;

        proc = opendir(proc_prefix);

        if (proc == NULL) {
                fprintf(stderr, "ERROR: Couldn't open /proc.\n");
        }
        
        for (e = readdir(proc); e != NULL; e = readdir(proc)) {
                if (isdigit(e->d_name[0])) {
                        setup_comm_fn(e->d_name, comm_fn);
                        fd = open(comm_fn, O_RDONLY);
                        if (fd > -1) {                                
                                n = read(fd, comm, COMM_SIZE);
                                close(fd);
                                for (i=0; i < n; i++) {
                                        if (comm[i] == '\n') {
                                                comm[i] = '\0';
                                                break;
                                        }
                                }
                                printf("%s: %s\n", e->d_name, comm);
                        } else {
                                printf("%s\n", e->d_name);
                        }
                }
        }
        
        result = closedir(proc);
        if (result) {
                fprintf(stderr, "ERROR: Couldn't close /proc.\n");
        }
}

void signal_handler(int the_signal)
{
        int pid, status;

        if (the_signal == SIGHUP) {
                fprintf(stderr, "Received SIGHUP.\n");
                return;
        }
        
        if (the_signal != SIGCHLD) {
                fprintf(stderr, "Child handler called for signal %d?!\n",
                        the_signal);
                return;
        }

        pid = wait(&status);

        if (pid == -1) {
                /* nothing to wait for */
                return;
        }
        
        if (WIFEXITED(status)) {
                fprintf(stderr, "\nProcess %d exited with status %d.\n",
                        pid, WEXITSTATUS(status));
        } else {
                fprintf(stderr, "\nProcess %d aborted.\n", pid);
        }
}

void run_program(char *args[], int background, char *stdout_fn,
                 char *path, char *envp[])
{
        pid_t pid;
        int fd, *ret_status = NULL;
        char bin_fn[BUFFER_SIZE];

        pid = fork();
        if (pid) {
                if (background) {
                        fprintf(stderr,
                                "Process %d running in the background.\n",
                                pid);
                } else {
                        pid = wait(ret_status);
                }
        } else {
                find_binary(args[0], path, bin_fn, BUFFER_SIZE);

                if (stdout_fn != NULL) {
                        fd = creat(stdout_fn, 0666);
                        dup2(fd, 1);
                        close(fd);
                }
                
                if (execve(bin_fn, args, envp)) {
                        puts(strerror(errno));
                        exit(127);
                }
        }
}

void set_shell_variable(char *variable, char *variable_value){
	if(shell_environ_number >= ARR_SIZE){
		printf("Not enough memory for extra shell variable\n");
		return;
	}

	shell_environ[shell_environ_number].variable = variable;
	shell_environ[shell_environ_number].variable_value = variable_value;
	shell_environ_number++;
	
}
char set_environment_variable(char **args, int nargs, char ***envp){
	//flag will hold the start index of the variable in args array
	char flag = 0;
	for(int i = 0; i < nargs; ++i){
		if(strchr(args[i], '=') != NULL){
			flag = 1;
			break;
		}
	}
	if(!flag)
		return 0;
	if(!strcmp(args[0], "export")){
                flag = 2;
        }else{
                flag = 1;
        }
	
	char variable[ARR_SIZE];
	char *cp = variable;
	char *it = args[flag - 1];
	//take all part before '=' sign
	while(*it != '\0' && *it != '='){
		*cp = *it;
		++cp;
		++it;
	}
	

	char variable_value[ARR_SIZE];
	cp = variable_value;

	//copy from the rest of the string except '='
	//it++ to skip the '='
	it++;
	strcpy(cp, it);
	cp += strlen(it);

	//copy from the rest of the args array, append the space if neccesary 
	for(int i = flag; i < nargs; ++i){
		it = args[i];
		*cp = ' ';
		cp++;
		strcpy(cp, it);
		cp += strlen(it);
	}
	if(flag == 2){
		//Set the environment variable
		setenv(variable, variable_value, 1);
		//Reassign the envp variable to the current environ
		*envp = environ;
	}else{
		set_shell_variable(variable, variable_value);
	}
	
	return flag;
}
void prompt_loop(char *username, char *path, char *envp[]) {
        char buffer[BUFFER_SIZE];
        char *args[ARR_SIZE];
        
        int background;
        size_t nargs;
        char *s;
        int i, j;
        char *stdout_fn;
        
        while(1){
                printf("%s $ ", username);
                s = fgets(buffer, BUFFER_SIZE, stdin);
                
                if (s == NULL) {
                        /* we reached EOF */
                        printf("\n");
                        exit(0);
                }
                
                parse_args(buffer, args, ARR_SIZE, &nargs); 
 
                if (nargs==0) continue;
                
                if (!strcmp(args[0], "exit")) {
                        exit(0);
                }
		
		if(set_environment_variable(args, nargs, &envp)){
			continue;
		}
                if (!strcmp(args[0], "plist")) {
                        plist();
                        continue;
                }

                background = 0;            
                if (strcmp(args[nargs-1], "&") == 0) {
                        background = 1;
                        nargs--;
                        args[nargs] = NULL;
                }

                stdout_fn = NULL;
                for (i = 1; i < nargs; i++) {
                        if (args[i][0] == '>') {
                                stdout_fn = args[i];
                                stdout_fn++;
                                printf("Set stdout to %s\n", stdout_fn);
                                for (j = i; j < nargs - 1; j++) {
                                        args[j] = args[j+1];
                                }
                                nargs--;
                                args[nargs] = NULL;
                                break;
                        }
                }
                
                run_program(args, background, stdout_fn, path, envp);
        }    
}

int main(int argc, char *argv[], char *envp[])
{
        struct sigaction signal_handler_struct;
                
        char *username;
        char *default_username = "UNKNOWN";
        
        char *path;
        char *default_path = "/usr/bin:/bin";
        
        memset (&signal_handler_struct, 0, sizeof(signal_handler_struct));
        signal_handler_struct.sa_handler = signal_handler;
        signal_handler_struct.sa_flags = SA_RESTART;
        
        if (sigaction(SIGCHLD, &signal_handler_struct, NULL)) {
                fprintf(stderr, "Couldn't register SIGCHLD handler.\n");
        }
        
        if (sigaction(SIGHUP, &signal_handler_struct, NULL)) {
                fprintf(stderr, "Couldn't register SIGHUP handler.\n");
        }
        
        username = find_env("USER", default_username, envp);
        path = find_env("PATH", default_path, envp);

        prompt_loop(username, path, envp);
        
        return 0;
}
