#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "functions.h"

void extract_replace_env_variable(Shell_environ* shell_environ, int num_shell_environ,char **str){
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
				char variable_value[ARR_SIZE];
				get_shell_environ(shell_environ,num_shell_environ, variable, variable_value);
				if(strlen(variable_value) != 0){
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
		printf("%c\n", *it);
	}
	*str = result;
}

void get_shell_environ(Shell_environ *shell_environ, int num_shell_environ,char *variable, char *result){
	for(int i = 0; i < num_shell_environ; ++i){
		if(!strcmp(variable, shell_environ[i].variable)){
			strcpy(result, shell_environ[i].variable_value);
			break;
		}		
	}
}

void set_shell_variable(Shell_environ *shell_environ, int *num_shell_environ, char *variable, char *variable_value){
    char result[ARR_SIZE];
    get_shell_environ(shell_environ, *num_shell_environ, variable, result);
    if(!strcmp(result, variable_value))
        return;
	if(*num_shell_environ >= ARR_SIZE){
		printf("Not enough memory for extra shell variable\n");
		return;
	}

	shell_environ[*num_shell_environ].variable = variable;
	shell_environ[*num_shell_environ].variable_value = variable_value;
	*num_shell_environ = *num_shell_environ + 1;
	
}

char set_environment_variable(Shell_environ *shell_environ, int *num_shell_environ, char **args, int nargs){
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
	}else{
		set_shell_variable(shell_environ, num_shell_environ,variable, variable_value);
	}
	
	return flag;
}