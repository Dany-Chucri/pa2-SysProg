#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glob.h>

#define BUFSIZE 1000
#define BATCHMODE 0
#define INTERACTIVEMODE 1

// IMPLEMENT BATCH MODE ALONGSIDE INTERACTIVE MODE AS SOON AS IT'S READY///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DONT FORGET TO SET THE LAST EXIT STATUS TO 1 FOR CD, PWD, OR ANY OTHER PROGRAMS THAT ATTEMPT TO RUN/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DO NOT USE A SET BUFFER SIZE AS DEFINED ABOVE, FIND A WAY TO READ IT IN INSTEAD (malloc)////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INSTEAD OF PRINTF, FOR CERTAIN PROGRAMS/FUNCTIONS MAKE SURE TO USE write() INSTEAD TO ALLOW OUTPUT REDIRECTION/PIPING///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//design notes:
//dealing with bad syntax, e.g., foo < < bar
//-> print an error message and skip to the next newline

int tokenize(char* buf, char** tokens) {
    char* ptr;
    const char delim = 32; //ASCII for white space
    int tokptr1 = 0; int tokptr2 = 0; //tokptr1 for traversing token array, tokptr2 for traversing characters in each token
    if(buf != NULL) {
        ptr = buf;
    }
    else {
        printf("*ERROR*");
    }

    //to check if first/last entered value is a special character
    char* p; p = buf;
    char lastVal; char firstVal; int firstCheck = 0;
    while(*p != '\0' && *p != 10) {
        if (*p != delim || *p != '\0' ) {//|| *p != '\n' || *p != 10) {
            lastVal = *p;
            if (firstCheck == 0) {
                firstVal = *p;
                firstCheck++;
            }
        }
        p++;
        while(*p == delim && *(p+1) != '\0') {
            p++;
        }            
    }
    if (lastVal == '|' || lastVal == '<' || lastVal == '>') return 1;
    if (firstVal == '|' || firstVal == '<' || firstVal == '>') return 1;


            while(*ptr == delim && *(ptr+1) != '\0') {  ////moves it up if it starts with a whitespace
                ptr++;
            }
    while (*ptr != '\0') {
        //printf("token at %d ptr at %c\n", tokptr1, *ptr);
        if (*ptr != delim) {
            if (*ptr == '|' || *ptr == '<' || *ptr == '>') {         
                if (tokptr1 > 0) {
                    if ((tokens[tokptr1-1][0] == '|' || tokens[tokptr1-1][0] == '<' || tokens[tokptr1-1][0] == '>')) return 1;
                    else if (*(ptr - 1) != delim){ 
                        tokens[tokptr1][tokptr2] = '\0'; 
                        tokptr1++; tokptr2 = 0;
                    }
                }
                else {
                    if (*(ptr - 1) != delim){ 
                        tokens[tokptr1][tokptr2] = '\0';
                        tokptr1++; tokptr2 = 0;
                    }
                }
                tokens[tokptr1][tokptr2] = *ptr; tokens[tokptr1][tokptr2+1] = '\0';
                ptr++;
                while(*ptr == delim) {
                    ptr++;
                }
                if (*ptr != delim) {
                    tokptr1++; tokptr2 = 0;
                }

            }
            else {
                tokens[tokptr1][tokptr2] = *ptr;
                tokptr2++;
                ptr++;
            }
        }
        else {
            while(*ptr == delim && *(ptr+1) != '\0') {
                ptr++;
            }
            tokens[tokptr1][tokptr2] = '\0';
            tokptr1++;
            tokptr2 = 0;
        }
    }
    tokens[tokptr1][tokptr2-1] = '\0';
    if (tokens[tokptr1][0] == '|' || tokens[tokptr1][0] == '>' || tokens[tokptr1][0] == '<') return 1;
    return 0;
}

void wildcards(char** tokens) { //returns the pathnames of files that are included in the wildcard
    char** globs;
    glob_t gstruct;
    int r = glob(tokens[0], GLOB_ERR, NULL, &gstruct); 

    if(r != 0) { //error check
        if(r == GLOB_NOMATCH) 
            fprintf(stderr, "No glob matches\n");
        else
            fprintf(stderr, "Glob Error");
        exit(1);
    }
    
    printf("Number of found filenames: %zu\n", gstruct.gl_pathc);
    globs = gstruct.gl_pathv;
    while(*globs) {
        printf("%s\n", *globs);
        globs++;
    }


    /*
    char buf[1000];
    memset(buf, (char) 0, 1000); 
    char* pathName = getcwd(buf, 1000);
    DIR* dir = opendir(getcwd(buf, 0));
    

    
    //int ptrIndex = 0;
    int asteriskIndex = 0;

    while(token[asteriskIndex] != '*') {   //Finds where the * char is
        //ptrIndex++;
        asteriskIndex++;
    }

    int ptrIndex = 0;


    
    while(token[ptrIndex]) {

    }
*/
}

/*
int executeProgram(char* pathName, char** tokens, char** tokenptr) { //executes specified program from command
    //set up arguments, redirection, piping here
    char** exeargs = malloc(sizeof(char*) * sizeof(tokens)); //create argument array to be passed into execv
    char** argsptr = tokenptr;
    memset(tokens, (char) 0, sizeof(tokens));
    for (int i = 0; i < sizeof(tokens); i++) {
        tokens[i] = malloc(sizeof(char) * (strlen(*argsptr) + 1));
        memset(tokens[i], (char) 0, (strlen(*argsptr) + 1));
        ++argsptr;
    }
    argsptr = tokenptr;
    int pipeFound = 0;
    while (strcmp(*tokenptr, "") != 0) {
        //first check for redirections/pipes before constructing argument aarray
        if (strcmp(*tokenptr, "<") == 0) {
            int fdIN = open(*(1 + tokenptr), O_RDONLY);
            dup2(fdIN, STDIN_FILENO);
            close(fdIN);
            ++tokenptr;
            break;
        }
        else if (strcmp(*tokenptr, ">") == 0) {
            int fdOUT = creat(*(1 + tokenptr), 0640);
            dup2(fdOUT, STDOUT_FILENO);
            close(fdOUT);
            ++tokenptr;
            break;
        }
        else if (strcmp(*tokenptr, "|") == 0) {
            pipeFound = 1;
            ++tokenptr;
            break;
        }
        else strcpy(*argsptr, *tokenptr);
        ++tokenptr;
        ++argsptr;
    }

    //set up child process, first check for piping
    int pid = fork();
    if (pid == -1) { //error check
        return 1;
    }
    else if (pid == 0) {
        //execv the arguments
        execv(pathName, exeargs);
        exit(EXIT_FAILURE); //error check
    }

    //for freeing exeargs array
    for (int i = 0; i < sizeof(exeargs); i++) {
        free(exeargs[i]);
    }
    free(exeargs);

    //checking status of child process
    int waitStatus;
    int tpid = wait(&waitStatus);   
    if (WIFEXITED(waitStatus)) return 0; //wait error check
    else return 1;
}*/

/*char* searchName(char** tokenptr) { //searches for a given file name in 1 of 6 directories

}*/

int main(int argc, char** argv) {
    int mode;
    if (argv[1] != NULL) mode = BATCHMODE; //batch mode
    if (argv[1] == NULL) { //interactive mode
        printf("Welcome to MyShell!\n"); 
        mode = INTERACTIVEMODE;
    }

    char buf[BUFSIZE];
    memset(buf, (char) 0, BUFSIZE);
    int sfd = STDIN_FILENO; //set input stream file descriptor

    if (mode == BATCHMODE) { //change input to the file passed as an argument for batch mode
        int bfd = -1; //batch file descriptor
        bfd = open(argv[1], O_RDONLY);
        if (bfd == -1) return EXIT_FAILURE;
        else sfd = bfd;
    }
    
    if (mode == 1) {
        printf("mysh> ");
        fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    int bytes = 0;
    while ((bytes = read(sfd, buf, BUFSIZE)) > 0) { //reading in input from stream and writing it to buf
        if(mode == INTERACTIVEMODE) {
            int tempstatus = 0; //return status of mysh

            char** tokens = malloc(sizeof(char*) * bytes);
            memset(tokens, (char) 0, bytes);
            for (int i = 0; i < bytes; i++) {
                tokens[i] = malloc(sizeof(char) * bytes);
                memset(tokens[i], (char) 0, bytes);
            }

            int valid = tokenize(buf, tokens); //parse command input inside buffer as an array of tokens
            if (valid == 1) { //check for valid command syntax
                printf("Inappropriate command syntax!\n");
                memset(buf, (char) 0, BUFSIZE); //fake flush the buffer
                for (int i = 0; i < bytes; i++) {
                    free(tokens[i]);
                }
                free(tokens);
                printf("!mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                continue;
            }

            //TOKEN PRINT TESTING 1
            /*int looper1 = 0; 
            while (strcmp(tokens[looper1], "") != 0) {
                printf("%s\n", tokens[looper1]);
                looper1++;
            }*/

            //look for home directory or wildcard replacement in the tokens first
            int looper = 0; 
            while (strcmp(tokens[looper], "") != 0) {
                if (tokens[looper][0] == '~' && tokens[looper][1] == '/') { //found a path relative to the home directory
                    char* path = tokens[looper];
                    memmove(path, path+1, strlen(path)); //remove the ~ character from the path name
                    char* homeDir = getenv("HOME"); //get home directory path
                    char* home = malloc(sizeof(char) * strlen(homeDir) + strlen(path) + 1); //this little section bellow is to avoid changing the value of getenv("HOME") in later calls, specifically in cd
                    //printf("%ld\n", strlen(homeDir));
                    home = strcpy(home, homeDir);
                    char* newPath = strcat(home, path);
                    int space = (strlen(home) + strlen(path) + 1);
                    for (int i = 0; i < bytes; i++) {
                        tokens[looper] = realloc(tokens[looper], sizeof(char) * (bytes + space)); //reallocate space for new token lengths, must realloc entire array to avoid pointers crossing into wrong regions
                    }
                    int i = 0;
                    for (i = 0; i < strlen(newPath); i++) { //place new path name as a part token
                        tokens[looper][i] = newPath[i];
                        //printf("%c", tokens[looper][i]);
                    }
                    tokens[looper][i] = '\0';
                    free(home);
                    //printf("\n");
                }
                //now look for * in tokens for wildcard replacement

                looper++; 
            }
            
            //TOKEN PRINT TESTING 2
            /*int looper2 = 0; 
            while (strcmp(tokens[looper2], "") != 0) {
                printf("%s\n", tokens[looper2]);
                looper2++;
            }*/

            int shellExit = 0; //tracker for exit command

            //begin interpretation of tokens and execution of commands
            char** tokenptr = tokens; //for traversal through tokens
            while (strcmp(*tokenptr, "") != 0) {
                if (strcmp(*tokenptr, "cd") == 0) { //execute cd command
                    if (strcmp(*tokenptr, tokens[0]) != 0) { //cd must be the first token in a command according to our implementation
                        printf("Inappropriate command! \"cd\" must be at the start of a command.\n");
                        tempstatus = 1;
                        break;
                    }
                    ++tokenptr;
                    //printf("the tokenptr is now at %s\n", *tokenptr);
                    if (strcmp(*tokenptr, "") == 0) //no arguments given, change to home directory
                    {
                        char* homeDir = getenv("HOME");
                        int success = chdir(homeDir);
                        if (success == -1) { //check if failed
                            perror("");
                            printf("failure to change to home env\n");
                            tempstatus = 1;
                        }
                        else printf("Succesfully changed to home directory, THIS IS ONLY FOR TESTING REMEMBER TO DELETE THIS\n");
                        break;
                    }
                    else {
                        int success = chdir(*tokenptr);
                        if (success == -1) { //check if failed
                            perror("");
                            tempstatus = 1;
                        }
                        else printf("Succesfully changed to specified directory, THIS IS ONLY FOR TESTING REMEMBER TO DELETE THIS\n");
                        break;
                    }
                }
                else if (strcmp(*tokenptr, "pwd") == 0) { //execute pwd command
                    if (strcmp(*tokenptr, tokens[0]) != 0) { //pwd must be the first token in a command according to our implementation, but its output can be piped/redirected
                        perror("Inappropriate command! \"pwd\" must be at the start of a command.\n");
                        tempstatus = 1;
                        break;
                    }
                    char buffer[1000];
                    memset(buffer, (char) 0, 1000); 
                    char* pathName = getcwd(buffer, 1000);
                    //check for redirection or piping first///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    printf("%s\n", pathName);
                    ++tokenptr;
                }
                else if (strcmp(*tokenptr, "exit") == 0) { //execute exit command
                    shellExit = 1;
                    ++tokenptr;
                } 
                else { //first check if the first token contains a /, indicating a path name to an executable program
                    int j = 0;
                    char slash = tokens[0][0];
                    while (slash != NULL) {
                        if (slash == '/') break; //found a pathname  
                        j++;
                        slash = tokens[0][j];
                    }
                    if (slash == '/') { //now execute program at pathname 
                        //now execute the program
                        char* pathName = tokens[0];
                        int childStatus = executeProgram(pathName, tokens, tokenptr);
                        if (childStatus == 1) { //failed to execute
                            printf("Program %s could not be executed\n", pathName);
                            tempstatus = 1;
                        }

                    }
                    else { //did not find a path name, so search in directories for a file w/ the specified bare name
                        char* pathName = searchName(tokenptr);
                        if (strcmp(pathName, "") == 0) { //could not find specified bare name
                            printf("Program %s could not be found\n", *tokenptr);
                            tempstatus = 1;
                        }
                        else {
                            char* pathName = *tokenptr;
                            int childStatus = executeProgram(pathName, tokens, tokenptr);
                            if (childStatus == 1) { //failed to execute
                                printf("Program %s could not be executed\n", pathName);
                                tempstatus = 1;
                            }
                        }
                    }
                }
                
               ++tokenptr; //TEMPORARY INCREMENT, DONT FORGET TO REMOVE////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }

            memset(buf, 0, BUFSIZE); //fake flush the buffer
            for (int i = 0; i < bytes; i++) {
                free(tokens[i]);
            }
            free(tokens);
            if (tempstatus == 1)
            {
                printf("!mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
            else {
                printf("mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
            if (shellExit == 1) { //check if we need to exit
                printf("exiting\n");
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}
