#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_COMMANDS 256
#define ERROR_STATUS 127


int executeCommand(char *command);

int main(int argc, char *argv[])
{
    //Declare variables
    char inputBuffer[MAX_INPUT]; //Initialize buffer to put input stream
    char bar[] = "|"; //Initialize delimiter for piping
    int numCommands;  //Count for number of commands entered
    int status = 0; //Running Status
    char *commands[MAX_COMMANDS];
    int finalPid;

    //Loop
    while(status!=-1) {
        status = 0;
        printf("jsh$");
        numCommands=0;
        finalPid=0;
        //Put user input into input buffer
        if(fgets(inputBuffer, MAX_INPUT, stdin)==NULL)
        {
            perror("FGets Error");
            exit(-1);
        }

        //Ignore empty inputs
        if(inputBuffer[0] !='\n' && inputBuffer[0] !='\0' && inputBuffer[0] !='\t')
        {
            //Use this approach for separating piped commands
            char* command = strtok(inputBuffer, bar);

            //If first word entered is exit, terminate shell
            if(strcmp(command, "exit")==0 || strcmp(command, "exit\n")==0)
                break;

            //Tokenize commands
            while(command!=NULL)
            {
                //If First Command -> Add to Commands and Increment Num Commands
                if(numCommands==0)
                {
                    commands[numCommands] = command;
                    numCommands++;
                }
                //If Not First Command
                else{
                    //If previous command doesn't end / -> Add new command
                    if(commands[numCommands-1][strlen(commands[numCommands-1])-1]!= '\\')
                    {
                        commands[numCommands] = command;
                        numCommands++;
                    }
                    //Else append to previous command
                    else{
                        //Reallocate memory to hold new string
                        // 2 for '|' and null terminator
                        size_t size = strlen(commands[numCommands - 1]) +  strlen(command) + 2;
                        char *newCommand = malloc(size);
                        strcat(newCommand,commands[numCommands - 1]);
                        strcat(newCommand, bar);
                        strcat(newCommand, command);
                        commands[numCommands - 1] = newCommand;
                    }
                }
                command = strtok(NULL, bar);
            }

            //Initialize pipes array
            int pipes[MAX_COMMANDS][2];
            //Iterate through all commands
            for(int i=0; i<numCommands; i++)
            {
                //Create pipe for command
                if (pipe(pipes[i]) == -1) {
                    perror("Pipe Error");
                    exit(-1);
                }
                //Create new process
                int rc = fork();
                if(rc<0){
                    perror("Fork Failed");
                    return -1;
                }
                //In Child Process
                else if(rc==0)
                {
                    //If not first command
                    if(i!=0)
                    {
                        //Close previous write end
                        close(pipes[i-1][1]);
                        //Adjust STDIN to read from previous pipe
                        if(dup2(pipes[i-1][0], STDIN_FILENO)==-1)
                        {
                            perror("Dup Error");
                            exit(-1);
                        }
                        close(pipes[i-1][0]);
                    }
                    //If not last command
                    if(i!=numCommands-1)
                    {
                        close(pipes[i][0]); //close child read pipe since we aren't reading from pipe

                        //Adjust STDOUT
                        if(dup2(pipes[i][1], STDOUT_FILENO)==-1)
                        {
                            perror("Dup Error");
                            exit(-1);
                        }
                        close(pipes[i][1]);
                    }
                    executeCommand(commands[i]);
                }
                //If parent process
                else {

                    //Close previous pipes since no longer needed
                    if(i!=0)
                    {
                        close(pipes[i-1][0]);
                        close(pipes[i-1][1]);
                    }

                    //Check for Final PID
                    if (i == numCommands - 1) {
                        close(pipes[i][0]);
                        finalPid = rc;
                    }
                }
            }

            // Wait for the final process to finish
            int waitStatus;
            if (waitpid(finalPid, &waitStatus, 0) < 0) {
                perror("Wait Error");
            }
            if(WEXITSTATUS(waitStatus)==ERROR_STATUS)
                printf("Invalid Command. Please try again.\n");
            if (WIFEXITED(waitStatus)) {
                printf("jsh status : %d\n", WEXITSTATUS(waitStatus));
            }

            // Wait for all child processes to avoid zombies
            for (int j = 0; j < numCommands; j++) {
                wait(NULL);
            }
        }

        // Clear Buffers after command execution
        memset(inputBuffer, 0, sizeof(inputBuffer));
        memset(commands, 0, sizeof(commands));
    }
    return 0;
}

/**
 *
 * @param command
 * @return
 * Function parses the command/args and sends to exec. Note this function DOES NOT fork but does call exec, so always
 * create a child process before calling this
 */
int executeCommand(char *command)
{
    char delim[] = " \n"; //Initialize delimiter we will use to spit the char array
    char *currentWord = strtok(command, delim); //Extract first word
    char *argBuffer[MAX_INPUT - 1];//Allocate arg buffer space
    int numArgs=1; //Counter for num arguments
    argBuffer[0]=currentWord;

    //Ignore Newline character
    if(command[0] != '\n' && command[0] != '\0' && command[0] != '\t')
    {
        currentWord = strtok(NULL, delim);  //Initialize pointer to second word

        //Loop through remaining words
        while(currentWord!=NULL)
        {
            argBuffer[numArgs] = currentWord;  //Add current word to arg buffer
            numArgs++;  //Increment arg counter
            currentWord = strtok(NULL, delim);  //Move to next word
        }

        if(execvp(argBuffer[0], argBuffer)==-1) {
            exit(ERROR_STATUS);
        }
    }
    return 1;
}
