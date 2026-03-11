#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
 *
 */
char **tokenize(char *line) {
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for (i = 0; i < strlen(line); i++) {

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
      token[tokenIndex] = '\0';
      if (tokenIndex != 0) {
        tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
        strcpy(tokens[tokenNo++], token);
        tokenIndex = 0;
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
  if(tokenIndex != 0){
	  token[tokenIndex] = '\0';
	  tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	  strcpy(tokens[tokenNo++],token);
  }

  free(token);
  tokens[tokenNo] = NULL;
  return tokens;
}

int main(int argc, char *argv[]) {
  char line[MAX_INPUT_SIZE];
  char **tokens;
  int i;
  char *filename = NULL;

  while (1) {
    /* BEGIN: TAKING INPUT */
    printf("minersh$ ");
    fflush(stdout);

    if(fgets(line,sizeof(line), stdin) == NULL){
	    //EOF - exit shell
	    printf("\n");
	    break;
    }
    // Remove trailing newline (fgets includes it)
    line[strcspn(line,"\n")] = '\0';
    /* END: TAKING INPUT */
    tokens = tokenize(line);
    // Checks for empty command (Enter)
    if (tokens[0] == NULL) {
      // Just free tokens and continu
      free(tokens[i]); 
      continue; // Go to next iteration
    }
    //Handler for exit command
    if(strcmp(tokens[0],"exit") == 0){
	    for(i = 0; tokens[i] != NULL;i++)
		    free(tokens[i]);
	    free(tokens);
	    break;
    }

    if (strcmp(tokens[0], "cd") == 0) {
      // Check argument count
      // NO directory specified
      if (tokens[1] == NULL) printf("cd: missing argument\n"); 
      //Too many arguments
      else if(tokens[2] != NULL)printf("cd: too many arguments\n");
      //Exactly one arguments
      else if(chdir(tokens[1]) != 0)printf("cd:"); 
      //Free tokens
      for(i = 0; tokens[i] != NULL; i++)free(tokens[i]);
      free(tokens);
      continue;
    }
    
    // Check for redirection ( > ) and pipe ( | )
    int redir_index = -1;
    int pipe_index = -1;

    for(i = 0; tokens[i] != NULL; i++){
	    if(strcmp(tokens[i],">") == 0)redir_index = i;
	    if(strcmp(tokens[i],"|") == 0)pipe_index = i;
    }
    // Handles PIPES
    if(pipe_index != -1){
	    //Validate syntax:
	    if(pipe_index == 0 || tokens[pipe_index + 1] == NULL){
		    write(STDERR_FILENO, "An error has occurred\n",22);
		    for(i = 0; tokens[i] != NULL; i++) free(tokens[i]);
		    free(tokens);
		    continue;
	    }
	    //Split tokes into left and right commands
	    printf("SPLIT TOKENS INTO LEFT AND RIGHT COMMANDS\n");

	    // Free tokens and continue
	    for(i=0;tokens[i] != NULL; i++) free(tokens[i]);
	    free(tokens);
	    continue;
    }
    // Handles REDIRECTION
    if(redir_index != -1){
	    // Redirection
	    if(tokens[redir_index + 1] == NULL || tokens[redir_index + 2] != NULL){
		    // No file after '>' or extra tokens after filename error
		write(STDERR_FILENO,"An error has occured\n",22);
		//Free and continue
		for(i = 0; tokens[i] != NULL; i++)free(tokens[i]);
      		free(tokens);
		continue;
	    }
	//Terminates command before '>'	
	// Now tokens[0 ... redir_index-1] is the command 
	char *filename = tokens[redir_index + 1];
	tokens[redir_index] = NULL;
    }
    pid_t pid = fork();
    if (pid < 0) {
	    write(STDERR_FILENO, "An error has occurred\n",22);
	    for(i = 0; tokens[i] != NULL; i ++)free(tokens[i]);
	    free(tokens[i]);
	    continue;
    } 
    if (pid == 0) {
      // Child process
      if (redir_index != -1){
	      //Open file for writing (Create if not exist, truncate)
	      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	      if(fd < 0){
		      write(STDERR_FILENO,"An error has occurred\n",22);
		      exit(1);
	      }
	     //Redirect stdout and stderr to the file 
	     if(dup2(fd,STDOUT_FILENO) < 0 || dup2(fd,STDERR_FILENO) < 0){
		     write(STDERR_FILENO, "An error has occurred\n",22);
		     exit(1);
	     }
	     close(fd);
      }

      //Execute command 
      execvp(tokens[0],tokens);

      // If here, execvp failed 
      write(STDERR_FILENO,"An error has occurred\n",22);
      exit(1);

    } else {
      // Parent process
      int status;
      // Wait for child to finish
      waitpid(pid, &status, 0);
    } 
    // Free allocated tokens
    for(i = 0; tokens[i] != NULL; i ++)free(tokens[i]);
    free(tokens[i]);
  }
  return 0;
}
