#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

// Set up constant variables for convenience
#define BUFFERSIZE 80
#define SHELLNAME "mini-shell"

// Execute a shell command
//
// param:  argvlist[] - an array of strings to store 
//                       the command and its arguments
void execCMD(char* argvlist[]){
    pid_t childPID = fork();

    if(childPID < 0){
        perror(SHELLNAME "execution failed (fork error)");
        return;
    }

    if(childPID == 0){
        // Execute command from child
        execvp(argvlist[0], argvlist);
        fprintf(stderr, SHELLNAME
                ": command not found--Did you mean something else?\n");
        exit(1);
    }else{
        wait(NULL);
    }
}

// Execute two commands with a pipe "|" between them
//
// params:  argvlist1[] - an array of strings to store 
//                        the first command and its arguments
//          argvlist2[] - an array of strings to store 
//                        the second command and its arguments
void execPipe(char* argvlist1[], char* argvlist2[]){
    // Set up file descriptors for pipe: 0 for read, 1 for write 
    int fd[2];  
    pid_t pid1, pid2; 
    if(pipe(fd) < 0){ 
        perror(SHELLNAME "pipe parsing error"); 
        return;
    }

    pid1 = fork(); 
    if(pid1 < 0){ 
        perror(SHELLNAME "pipe parsing failed (fork error)"); 
        return; 
    } 
    if(pid1 == 0){ 
        // Execute first command from child and write pipe
        close(1);
        close(fd[0]); 
        dup2(fd[1], STDOUT_FILENO); 
        close(fd[1]); 
        execvp(argvlist1[0], argvlist1); 
        fprintf(stderr, SHELLNAME
                ": command not found--Did you mean something else?\n"); 
        exit(1); 
    }else{
        pid2 = fork(); 
        if(pid2 < 0){ 
            perror(SHELLNAME "pipe parsing failed (fork error)"); 
            return; 
        } 
        // Execute second command from another child and read pipe
        if(pid2 == 0){ 
            close(0);
            close(fd[1]); 
            dup2(fd[0], STDIN_FILENO); 
            close(fd[0]); 
            execvp(argvlist2[0], argvlist2);
            fprintf(stderr, SHELLNAME
                    ": command not found--Did you mean something else?\n"); 
            exit(1); 
        }else{ 
            close(fd[0]);
            close(fd[1]);
            wait(NULL);
            wait(NULL);
        } 
    }
}

// Parse two shell commands with a pipe "|" between them
//
// params:  pipelist[] - an array of strings to store 
//                        the two commands
//              command - the command to parse
void parsePipe(char* pipelist[], char* command){
    char* token = strtok(command, "|");
    
    if(token == NULL){
        return;
    }
    
    pipelist[0] = token;
    token = strtok(NULL, "|");
    pipelist[1] = token;
}

// Execute a shell command
//
// params:  argvlist1[] - an array of strings to store 
//                        the command and its arguments
//              command - the command to parse
void parse(char* argvlist[], char* command){
    int i = 0;
    char* token = strtok(command, " ");

    while(token != NULL){
        argvlist[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    argvlist[i] = NULL;
}

// Execute the built-in "cd" command (change directory)
//
// param:  path - the path to change to 
void cdCMD(char* path){
    if(chdir(path) != 0){
        perror(SHELLNAME ": cd");
    }
}

// Execute the built-in help command
// Print a list of available commands
void helpCMD(){
    printf("====== MINI SHELL HELP ======\n"
          "List of commands supported:\n"
          "> cd: change directory\n"
          "> help: print a list of available commands\n"
          "> exit: terminate the shell\n"
          "> guess: play a little game for guessing random numbers\n"
          "> other commands available in bash shell\n"
          "> basic pipe handling support (one pipe)\n"); 
}

// Execute the built-in exit command
// Terminate the mini shell
void exitCMD(){
    exit(0);
}

// Play the built-in guess game for a round
// params:  i - the index of number of a round
//          numGuess[] - an array to store the score of each round
void guessingGameRound(int i, int numGuess[]){
    int number, guess;

    // Randomly generate a number from 1 to 10
    srand((unsigned)time(NULL));
    number = rand() % 10 + 1;

    // Tell the user to make a guess
    puts("Welcome to the guessing game");
    puts("The rule is simple: guess a random number and see how long "
         "it takes for you to get the number picked by CPU.\n"
         "Play the game for a total of 5 rounds and you will see your "
         "scores for each round.\nHave fun!");
    puts("==========================");
    puts("CPU Says: Pick a number 1-10");
    puts("==========================");

    // Start guessing the number and keep track of 
    // the number of guesses
    while(1){
        // Read the guess from the user
        // Repeatly prompt for guess if the user does 
        // not enter a valid number (single integer)
        while(1){
            printf("Make a guess: ");
            if(scanf("%d", &guess) == 1){
                break;
            }
            puts("Please enter an integer");
            getchar();
        }
        numGuess[i]++;

        // Check whether the user's guess is correct
        if(guess < number){
            puts("The correct number is higher!");
        }else if(guess > number){
            puts("The correct number is lower!");
        }else{
            puts("You got it!");
            break;
        }
    }
}

// Execute the built-in guess command
// Play a little game for guessing a random number
void guessingGame(){
    int i;
    int numGuess[5] = {0};

    // Play the game for 5 rounds
    for(i = 0; i < 5; i++){
        guessingGameRound(i, numGuess);
    }

    // Announce the result (summary of the number of guesses 
    // the user made over 5 games
    puts("=================================================");
    puts("|Here are the results of your guessing abilities|");
    puts("=================================================");
    for (i = 0; i < 5; i++) {
        printf("Game %d took you %d guesses\n", i, numGuess[i]);
    }
}

// Execute the built-in commands
// param:  command - the command to parse
int execBuiltin(char* command){
    void (*builtinCMD[])() = {helpCMD, exitCMD, guessingGame};
    char* builtinCMDList[] = {"help", "exit", "guess"};
    int i;
    for(i = 0; i < 3; i++){
        if(strcmp(command, builtinCMDList[i]) == 0){
            builtinCMD[i]();
            return 1;
        }
    }
    return 0;
}

// Create a signal handler
// param:  sig - the signal number
void sigint_handler(int sig){
    printf(SHELLNAME " terminated\n");
    exit(0);
}

int main(){
    signal(SIGINT, sigint_handler);

    while(1){
        // Get command from input
        char* command;
        size_t bufferSize = BUFFERSIZE;
        printf(SHELLNAME "> ");
        int inputSize = getline(&command, &bufferSize, stdin);
        command[inputSize - 1] = 0;

        // Catch Ctr-D and terminate the shell
        if(inputSize == -1){
            printf(SHELLNAME " terminated\n");
            exit(0);
        }
        // Catch empty command
        if(strcmp(command, "") == 0){
            continue;
        }
        // Catch built-in commands
        if(execBuiltin(command)){
            continue;
        }

        // Check if pipe exists in the command and execute command accordingly
        if(strstr(command, "|") == NULL){
            char* argvlist[BUFFERSIZE + 1];
            parse(argvlist, command);
            if(argvlist[0] == NULL){
                continue;
            }
            if(strcmp(argvlist[0], "cd") == 0){
                cdCMD(argvlist[1]);
            }else{
                execCMD(argvlist);
            }
        }else{
            char* pipelist[2];
            char* argvlist1[BUFFERSIZE + 1];
            char* argvlist2[BUFFERSIZE + 1];
            parsePipe(pipelist, command);
            parse(argvlist1, pipelist[0]);
            parse(argvlist2, pipelist[1]);
            execPipe(argvlist1, argvlist2);
        }
    }    
    return 0;
}
