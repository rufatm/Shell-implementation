/*
getline vs readline 
put dup2 for pipes 
sigint 
*/

#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

struct sigaction act_open;



 
#define MAX_COUNT 100
//#define MAX_LINE 80

#define MAX_CMD_SIZE 64
#define MAX_CMD_COUNT 200

#define MAX_ARG_COUNT 10
#define MAX_ARG_SIZE 100

typedef struct _cmd_t {
	int argc;
	char argv[MAX_ARG_COUNT][MAX_ARG_SIZE]; 
	int has_pipe;
	int stdin_redirect; 
	int stdout_redirect; 
	int stderr_redirect; 
	char* input_file; 
	char* output_file;   
} cmd_t;
 
 
 

 
static int command_count, pipecount;
static int piped_cmd_index;
static int in_background;

static int fd_pipe[MAX_COUNT][2];

char** full_commands;
static cmd_t cmd_structs[MAX_CMD_COUNT];
 
 
static int background_processes[MAX_COUNT];
int b_size = 0;
 
int count_commands (char*); 
int count_pipes (char*);

int parse_line (char*);
char* parse_ampersand(char* ); 
void parse_semicolon(char*);
void parse_spaces(int);
void print_cmd_struct(cmd_t);
int check_redirects();
void execute(int);





void  INThandler(int sig)
{ 
	int z; 
	for (z = 0; z < b_size; z++)
	{
		kill(background_processes[z],SIGTERM);
		background_processes[z] = -1;
	}
	b_size = 0;
}



int main()
{
	char* line;

act_open.sa_flags = 0;
    
  // Create a mostly open mask -- only masking SIGINT
   

	while (1) 
	{ 
	 	sigemptyset(&act_open.sa_mask);
    	sigaddset(&act_open.sa_mask, SIGINT);

    	act_open.sa_handler = INThandler;
    	sigaction(SIGINT, &act_open, 0);

		init();
		if (isatty(STDIN_FILENO))
		{
  			line = readline("myshell>"); 
  			if (line == NULL) 
  			{
  				printf("\n");
  				break;
  			}
  		}
  		else 
  		{
  			line = readline("");
  			if (line == NULL) 
  			{
  				//printf("Nothing else to read in the file\n");
  				break;
  			}
  		}
  		if (*line == '\0')
  			continue;
  		parse_line(line); 
  		int i;
  		for (i = 0; i < command_count; i++) 
  		{ 
  			pipecount = 0;
  			init();
  			parse_spaces(i); // create a list of cmd_t structs 
		//	printf("\n");
			
			
			
  			int check = check_redirects(); 
  			if (check == 0) 
  			{
  				printf("aborting the command...\n");
  				free(full_commands);
  				free(line);
  				break;
  			}
  			
  			
  			
 			execute(i); 
  		}
  		
  		free(full_commands);
  		free(line);
	}
}



 
 int count_commands(char* line)
 { 
 	int len = strlen(line); 
 //	printf("len is %d \n", len);
 	if (len == 1) 
 		return 0;
 	int i; 
 	int cmd_count = 0;
 	for (i = 0; i < len; i++) 
 	{ 
 		if (*line++ == ';')
 			cmd_count++;
 	}
	return cmd_count+1;
 	
 }
 
 int count_pipes(char* line) 
 { 
 	int len = strlen(line); 
 	if (len == 1) 
 		return 0;
 	int i; 
 	int pipe_count = 0; 
 	for (i = 0; i < len; i++ ) 
 	{ 
 		if (*line++ = '|') 
 			pipe_count++;	
 	} 
 	if (pipe_count != 0) 
 		return pipe_count + 1; 
 	else 
 		return pipe_count;
 }
 
int init() 
{ 
	piped_cmd_index  = 0;
	in_background = 0;
	
	return 1;
}


int parse_line(char* line) 
{ 
	in_background = 0;
	char* amp = parse_ampersand(line);
	command_count = count_commands(amp);
	full_commands = (char**) malloc (sizeof(char*) * command_count);
	parse_semicolon(amp);
	return 1;
	
		
}

char* parse_ampersand(char* line) 
{ 
	int len = strlen(line);
	char* line_without_amp = strtok(line, "&\n");
 	if (len > strlen(line_without_amp)) 
 	{ 
 		//printf("Ampersand detected! Executing this command in the background...\n"); 
 		in_background = 1;
 	}
 	return line_without_amp;
 	
}

void parse_semicolon(char* line) 
{ 
	// Note: may need to replace delimeter ";" with "; "
	int i = 0;
	char* token = strtok(line,";"); 
	full_commands[i++] = token; 
	while (token != NULL) 
	{ 
		token = strtok(NULL, ";"); 
		full_commands[i++] = token;
	}
	//printf("Parsed all the semicolons\n"); 
	//printf("command_count %d \n", command_count);
}

void parse_spaces(int n) 
{ 
	piped_cmd_index = 0;
	//printf("\nParsing command %s of length %d \n", full_commands[n], strlen(full_commands[n])); 
	char* copy = malloc(sizeof(char*) * strlen(full_commands[n])); 
	strcpy(copy, full_commands[n]);
	char* token = strtok(copy, " \n"); 
//	printf("The name of the program is %s of length %d \n", token, strlen(token));
	int i = 1;
	//token[strlen(token)] = 0;
	strcpy(cmd_structs[piped_cmd_index].argv[0],token);
	//printf("PROGRAM %s \n", cmd_structs[piped_cmd_index].argv[0]);

	int flag  = 0;
	int pipe = 0;	
	cmd_structs[piped_cmd_index].argc = 1;
	while (token != NULL) 
	{ 
		token = strtok(NULL," \n"); 
		
	 	//printf("got it\n");
	/*
		if (token != NULL)
			printf("token %s of length %d\n", token, strlen(token));
 		else 
 			printf("token is NULL\n");
 		*/
 		if (token != NULL) 
 		{
 			if (token[0] == '>' || (token[0] == '1' && token[1] == '>'))
 			{ 
 			//	printf("> \n\n\n\n");
 				cmd_structs[piped_cmd_index].stdout_redirect++;
 				token = strtok(NULL, " "); 
				//strcpy(cmd_structs[piped_cmd_index].output_file,token);
				cmd_structs[piped_cmd_index].output_file = token;
 			}
 			else if (token[0] == '<') 
 			{
 			 	//printf("< \n");
 				cmd_structs[piped_cmd_index].stdin_redirect++;; 
 				token = strtok(NULL, " \n"); 
				cmd_structs[piped_cmd_index].input_file = token;
 			}
 			else if (token[0] == '2' && token[1] == '>') 
 			{
 			 //	printf("2> \n");
 				cmd_structs[piped_cmd_index].stderr_redirect++;
 				token = strtok(NULL, " \n"); 
				cmd_structs[piped_cmd_index].output_file = token;
 			}
 			else if (token[0] == '&' && token[1] == '>') 
 			{ 
 			 //	printf("&> \n");
 				cmd_structs[piped_cmd_index].stderr_redirect++; 
 				cmd_structs[piped_cmd_index].stdout_redirect++;
 				token = strtok(NULL, " \n"); 
				cmd_structs[piped_cmd_index].output_file = token;
 			} 
 			else if (token[0] == '|')
 			{ 
 			//	printf("PIPE\n");
 				pipecount++;
 				cmd_structs[piped_cmd_index].has_pipe = 1; 
 				token = strtok(NULL, " \n"); 
 				cmd_structs[piped_cmd_index].stdout_redirect++;
				cmd_structs[piped_cmd_index+1].stdin_redirect++;
				strcpy(cmd_structs[++piped_cmd_index].argv[0],token);
				cmd_structs[piped_cmd_index].argc = 1;
				

				i = 1;
 			}
 			else 
 			{
 				strcpy(cmd_structs[piped_cmd_index].argv[i++],token);
 				cmd_structs[piped_cmd_index].argc++; 
 			}
 		}	
	}
	
}




void initialize_cmd_struct(cmd_t* s)
{ 
	//s->argv = NULL; 
	s->stdin_redirect = 0; 
	s->stdout_redirect = 0; 
	s->stderr_redirect = 0;
	s->has_pipe = 0; 
	s->input_file = NULL; 
	s->output_file = NULL;
}


int check_redirects() 
{ 
	int j;
	for (j = 0; j <= piped_cmd_index; j++) 
  	{ 
  		if (cmd_structs[j].stdin_redirect > 1)
  		{ 
  			printf("There are too many input redirects!\n");
  			return 0;
  		}
  		if (cmd_structs[j].stdout_redirect > 1) 
  		{ 
  			printf("There are too many output redirects!\n"); 
  			return 0;
  		}
  		if (cmd_structs[j].stderr_redirect > 1) 
  		{ 
  			printf("There are too many error redirects!\n");
  			return 0;
  		}
  	}	
  	return 1;
}



void print_cmd_struct(cmd_t x) 
{ 
//	int l = x.argc;
//	int i; 
//	printf("argc = %d\n",l);
	/*
	for (i = 0; i < l; i++)
	{
		printf("argv[%d] = %s\n",i,x.argv[i]); 
		
	}
	*/
	/*
	printf("has pipe = %d\n", x.has_pipe);
	printf("stdin_redirect = %d\n", x.stdin_redirect);
	printf("stdout_redirect = %d\n", x.stdout_redirect); 
	printf("stderr_redirect = %d\n", x.stderr_redirect); 
	printf("input_file = %s\n", x.input_file); 
	printf("output_file = %s\n", x.output_file);
	*/
}



void execute(int ind) 
{ 
	int fd[piped_cmd_index + 1][2]; 
	int pid[MAX_COUNT]; 
	int j;
	
	
	//printf("pipecount %d \n", pipecount);
	int z; 
	for (z = 0 ; z <= pipecount; z++) 
		pipe(fd_pipe[z]);
	z = 0;
	

	
  	for (j = 0; j <= piped_cmd_index; j++) 
  	{ 
  	
  		if (cmd_structs[j].stdin_redirect) 
		{ 
	//		printf("checking stdin_redirect for %d \n",j);
			if (j ==0 || (cmd_structs[j-1].has_pipe == 0))
			{
				fd[j][0] = open(cmd_structs[j].input_file, O_RDONLY);
				if (fd[j][0] < 0)
				{ 
					printf("error opening the file...\n"); 
					break;
				}
			}
		}
			
		if ( (cmd_structs[j].stdout_redirect || cmd_structs[j].stderr_redirect) && (cmd_structs[j].has_pipe == 0 )) 
		{
		//	printf("opening the output file... \n");
			fd[j][1] = open(cmd_structs[j].output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			//printf("Opened the output file with file descriptor %d \n", fd[j][1]);
		} 
  	
  		print_cmd_struct(cmd_structs[j]);
  	//	printf("forking...\n");
  		
  		
  		if ((pid[j] = fork()) == 0) 
 		{ 
 		
 			//if (cmd_structs[j].has_pipe)
 			//	pipe(fd_pipe[j]);
 		
 			
 		
 			int i_redirected= 0;
 			int o_redirected = 0;
 			if (j == 0)
 			{
 				if (cmd_structs[j].has_pipe)
 				{
 			//	printf("redirecting output to pipe for j == 0\n");
 					dup2(fd_pipe[j][1],STDOUT_FILENO);
 					o_redirected = 1;
 					//close(fd_pipe[j][0]);
 				}
 					// redirect the output of the pipe
 			} 
 			else if (j > 0 && j < piped_cmd_index)
 			{ 
 				if (cmd_structs[j].has_pipe)
 				{
// 				 	printf("redirecting output for cmd %d \n",j);

 					o_redirected = 1;
 					dup2(fd_pipe[j][1],STDOUT_FILENO);
 					//close(fd[][])
 					// rediret ouput to pipe 
 				}
 				if (cmd_structs[j-1].has_pipe) 
 				{
 			//	 	printf("redirecting input for cmd %d \n",j);

 					i_redirected = 1;
 					dup2(fd_pipe[j-1][0],STDIN_FILENO);
 					//close(fd_pipe[j-1][1]);
 					// redirect the input to pipe
 				}
 				//if (i_redirected && !o_redirected)
 			//		close(fd_pipe[j][1]);
 			//	if (o_redirected && !i_redirected)
 			//		close(fd_pipe[j][0]);
 			}
 			else 
 			{ 
 				if (cmd_structs[j-1].has_pipe)
 				{
 				//	printf("redirecting input for cmd %d \n",j);
					i_redirected = 1;	
 				 	dup2(fd_pipe[j-1][0],STDIN_FILENO);
 				 }
 					//redirect the input
 			}
 			
 			int ste= 0; 
 			int sti = 0; 
 			int sto =0;
 			if (cmd_structs[j].stdin_redirect && !i_redirected) 
			{ 
				dup2(fd[j][0],STDIN_FILENO);
				sti = 1;
			}
				 
			if (cmd_structs[j].stdout_redirect && !o_redirected) 
			{ 
			//	printf("printing to file\n");
				dup2(fd[j][1], STDOUT_FILENO);
				sto = 1;
			}
			
			if (cmd_structs[j].stderr_redirect) 
			{ 
				dup2(fd[j][1], 2);
				ste = 1;
			}
			// close unused file descriptors 
			
			
			//close(fd[j][0]); 
			//close(fd[j][1]); 
			
			
 			char* temp[cmd_structs[j].argc]; 
 			int k; 
 			for (k = 0; k < cmd_structs[j].argc; k++) 	
 			{ 
 				temp[k] = cmd_structs[j].argv[k];
 				//temp[k][strlen(temp[k])] = '\0'; 				
 			//	printf("temp %s of length %d \n", temp[k], strlen(temp[k]));
 			}
 			//printf("first %s \n", cmd_structs[j].argv[0]); 	

/*
		
				if (execvp(temp[0],temp) < 0 ) 
				{
 					printf("Error: Failed to exec the child %s with aruments \n", cmd_structs[j].argv[0]); 
					break;
				}
 */			
 			
 			if (cmd_structs[j].argc == 1) 
 			{ 
 			if (execvp(temp[0], NULL) < 0)
 			{
 				printf("Error: Failed to exec the child %s with aruments \n", cmd_structs[j].argv[0]); 
 				 close(pid[j]);
 				 break;
 			}
 			//free(temp);
 			
 		
 			}				
 			else 
 			{
 				if (execvp(temp[0], temp) < 0)
 				{
 					printf("Error: Failed to exec the child %s with aruments \n", cmd_structs[j].argv[0]); 
 					close(pid[j]);
 					break;
 				}
 			//free(temp);
 			}
 		
 		
 		} 
 		//else 
 		//{
 			if ( (ind == (command_count-1)) && in_background==1) 
 			{
 				background_processes[b_size++] = pid[j];
 			}
 			else
 			{ 
 			//printf("waiting at %d \n",j);
 				int status;
 				waitpid(pid[j],&status,0);
 				//print_cmd_struct(cmd_structs[j]);
 			//	printf("done waiting at %d \n", j);
 			}
  			//initialize_cmd_struct(&cmd_structs[j]);
  		//}
  		//printf("out\n");
  	}
  	for (j = 0; j <= piped_cmd_index; j++) 
  	{ 
  	  	initialize_cmd_struct(&cmd_structs[j]);
	}
	for (j = 0; j <= pipecount; j++) 
	{ 
		close(fd_pipe[j][0]); 
		close(fd_pipe[j][1]);
	}
  	
}	

