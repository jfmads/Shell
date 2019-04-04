#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include <fcntl.h>

void handle_commands(char**, int, int);
//void handle_pipe_commands(char**,char**, int, int);
void do_redirect(int, char**);
int pipe_check(char*);
int redirection_check(char*);
void shell_loop();
int tokenize_input(char*, char**, char*);

#define MAXSTRING 100
#define MAXCHAR 20
#define MAXCMDS 20

/*	
	This is a program that mimics a shell by using functions 
	available in the C library. Built-in and simple functions are both utilized.
*/
int main(){

	system("clear");
	printf("Welcome to JOEY'S PARTY SHELL! \n\n");
	shell_loop();

	return 0;
}
							/* HELPER FUNCTIONS */

/*
	This function handles the I/O redirection. It accepts a string that notifies it which operation to perform
	and a pointer to an array of char pointers of commands. 
*/
void do_redirect(int redi, char** cmds){
	char* cmds_before[MAXCMDS];
	char* cmds_after[MAXCMDS];
	int saved_std;
	int file_id;
	int i;
	int j;

	if (redi == 1){												// if the redi var is equal to 1 -- ouput to a file
		for(i = 0; strcmp(*(cmds+i),">") != 0; i++)				// gather arg
			*(cmds_before+i) = *(cmds+i);
		*(cmds_before+i) = NULL;
		i++; 													// increment past the ">"
	
		for (j = 0; *(cmds+i) != NULL; j++, i++)
				*(cmds_after+j) = *(cmds+i);
		*(cmds_after+j) = NULL;

		saved_std = dup(1);
		file_id = open(*cmds_after, O_CREAT| O_WRONLY, 0666);	// write a file with permisions
		close(1);
		dup(file_id); 											// duplicate file descriptor in slot 1
		close(file_id);

		if (fork() == 0){										// Handles all of the simple commands, forks a child and does procedure
			execvp(*cmds_before, cmds_before);					// executes the process given
			printf("%s: command not found\n", *cmds_before);	// this line will be reached when the process cannot be found
			exit(1);											// should not reach this line
		}else{
			wait(0);
		}
		fflush(stdout);
		dup2(saved_std, 1);
	}

/*
	if (redi == 2){										// input a file

		for(i = 0; strcmp(*(cmds+i),"<") != 0; i++)
			*(cmds_before+i) = *(cmds+i);
		*(cmds_before+i) = NULL;
		i++; 											//increment past the "<"
	
		for (j = 0; *(cmds+i) != NULL; j++, i++)
				*(cmds_after+j) = *(cmds+i);
		*(cmds_after+j) = NULL;

		saved_std = dup(1);
		file_id = open(*cmds_after, O_RDONLY);
		close(1);
		dup(file_id);
		close(file_id);

		if (fork() == 0){								// Handles all of the simple commands, forks a child and does procedure
			execvp(*cmds_before, cmds_before);			// executes the process given
			printf("%s: command not found\n", *cmds);	// this line will be reached when the process cannot be found
			exit(1);									// should not reach this line
		}else{
			wait(0);
		}
		fflush(stdout);
		dup2(saved_std, 1);
	}
*/
	return;
}

/*
	This function accepts an array of character pointers. That array is
	dereferenced in multiple ways and utilized in both built-in and 
	simple linux commands.
*/
void handle_commands(char** cmds, int bkgrnd, int redi){
	char cwd[MAXSTRING];
	
	if (redi < 0){											// if no redirection
		/* cd */				/*  BUILT-IN COMMANDS */
		if (strcmp(*cmds, "cd") == 0){						// built in command change directory
			if (chdir(*(cmds+1)) == 0){						// chdir returns a 0 when it is a successful change
				getcwd(cwd, sizeof(cwd));					// gets the current working directory and sets it to cwd 
				printf("%s\n", cwd);						// prints out the current working directory
			} else
				perror("");
			return;											// returns to loop after procedure is complete
		}
		/* exit */
		if (strcmp(*cmds, "exit") == 0){					// kills the parent terminal, not very safe??
			printf("\n\n... exiting ...\n\n");
			sleep(1);		
			kill(getppid(), SIGKILL);
	
			return;
		}
	
		if (bkgrnd == 0) {		/* background */			
			if (fork() == 0){								// Handles all of the simple commands, forks a child and does procedure
				execvp(*cmds, cmds);						// executes the process given
				printf("%s: command not found\n", *cmds);	// this line will be reached when the process cannot be found
				exit(1);									// should not reach this line
			}
		} else {				/* foreground */		 		
			if (fork() == 0){								
				execvp(*cmds, cmds);						
				printf("%s: command not found\n", *cmds);
				exit(1);									
			}else{
				wait(0);									// parent waits for child
			}
		}
	} else {
		do_redirect(redi, cmds); // do redirection
	}
}

/*
	This function handles the decision making of whether a string has a Pipe.
	A char pointer is accepted.
*/
int pipe_check(char* is){
	char* pipe = strstr(is,"|");
	//char* append_out = strstr(is,">>");

	if (pipe != NULL)
		return 0;

	return 1;
}

/*
	This function handles the decision making of whether a string has I/O redirection.
	A char pointer is accepted.
*/
int redirection_check(char* is){
	char* output = strstr(is,">");
	char* input = strstr(is,"<");
	//char* append_out = strstr(is,">>");

	if ((output != NULL) && (input != NULL))	// input and output redirection
		return 0;
	else if (output != NULL)
		return 1;
	else if (input != NULL)
		return 2; 
	
	return -1;
}

/* 
	This function is the main loop of the program. It continues to 
	execute until exit is read in. 
*/
void shell_loop(){
	char instream[MAXSTRING];								// the char array(string) stream from standard input, specified from the terminal
	char* commands[MAXCMDS];								// the array of pointers to the command strings	
	char* pipe1[MAXCMDS];									// the array of pointers before the pipe (if there is a pipe)
	char* pipe2[MAXCMDS];									// the array of pointers after the pipe (if there is a pipe)
	int redi = -1;											// will hold an integer value corresponding to a redirection type
	int bkgrnd = 1;											// integer value to determine foreground or background, initially false
	int pi = 1;												// integer value to determine if there is a pipe or not, initially false
	int i = 0;												// integer to traverse arrays
	
	do {
		printf(">: ");										// terminal prompt
		fgets(instream, MAXSTRING, stdin);					// gathers the input stream from the terminal			
		redi = redirection_check(instream);					// checks for input output redirection <>, <, >, >> 
		pi = pipe_check(instream);							// checks for pipes

//		printf("\n%d\n", redi);

		if (pi == 0){												// if there is a pipe in the string
			tokenize_input(instream, commands, "|");  				// tokenizes the input stream based on the pipe
			bkgrnd = tokenize_input(*commands, pipe1, " \n");		// tokenizes the first string before the pipe
			// maybe do something with bkgrnd?
			bkgrnd = tokenize_input(*(commands+1), pipe2, " \n");	// tokenizes the second string after the pipe
		
			//for(i = 0; *(pipe1+i) != NULL; i++)
			//	printf("%s\n",*(pipe1+i));

				int pipefd[2]; 									// 0: read-end-pipe, 1: write-end-pipe
				pipe(pipefd);

				if (fork() == 0){								// Handles all of the simple commands, forks a child and does procedure
					execvp(*pipe1, pipe1);						// executes the process given
					printf("%s: command not found\n", *pipe1);	// this line will be reached when the process cannot be found
					exit(1);									// should not reach this line
				}






		printf("\n%d\n", redi);
/*				if (fork() == 0) {
					close(pipefd[0]); 			// closes the read end of the pipe	
					dup2(pipefd[1], 1); 		// connects write end of pipe to stdout
					execvp(pipe1[0], pipe1); 			// [cat pipe.c] writes to pipe	
				} else if(fork() == 0) {
					close(pipefd[1]);  			// closes write end of pipe
					dup2(pipefd[0], 0); 			// connect read end of pipe to stdin

					if ( redi < 0)
					printf("lessthan\n");//	execvp(pipe2[0], pipe2); 			// [cat pipe.c] writes to pipe	
					//}else 
						//do_redirect(redi, pipe2);
				}
	
				close(pipefd[0]);
				close(pipefd[1]);
				wait(NULL);
				wait(NULL);*/
		}else{
			bkgrnd = tokenize_input(instream, commands, " \n");  		// tokenizes the input stream and adds them to the commands array
			handle_commands(commands, bkgrnd, redi);	
		}

	} while(1);
}

/* 
	This function accepts an array of chars and an array of char pointers. 
	The char array(string) is tokenized and passed into the array of char pointers.
	Returns the whether it will be a foreground or background process.
*/
int tokenize_input(char* is, char** cmds, char* delim){
	char* token = strtok(is, delim); 						// tokenizes the inputstream string by spaces and new lines, the token has a max length of 20char
	int len = 0;											// integer that will be used to traverse the commands array

	while(token != NULL){									// while the token does not point to a NULL value	
		cmds[len] = token;									// set string value of token to a position in the commands array
		len++;												// increase position
		token = strtok(NULL, delim);						// by passing NULL as the first argument it will continue to parse the same string
	}

	if (strcmp(*(cmds+(len-1)), "&") == 0) {				// will be 0 if true, 1 if false
		cmds[len - 1] = NULL;								// ignore the ampersand
		return 0;											// return that it is a background process
	} 

	*(cmds+len) = NULL;										// adds a NULL value to the end of the array
	return 1;												// returns that it will be a foreground process
}
































