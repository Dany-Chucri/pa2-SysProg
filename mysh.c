#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glob.h>

#define BUFSIZE 4096
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

int largestString (char** strings) { //returns length of largest string in a string array
    char** ptr = strings;
    int largest = 0;
    while (strcmp(*ptr, "") != 0) {
        if (strlen(*ptr) > largest) largest = strlen(*ptr);
        ++ptr;
    } 
    largest++; //for the \0
    return largest;   
}

int numberOfTokens(char** tokens) {
    int num = 0;
    char** ptr = tokens;
    while (strcmp(*ptr, "") != 0) {
        num++;
        ++ptr;
    } 
    return num;
}

int numberOfArgs(char** tokens) {
    int num = 0;
    char** ptr = tokens;
    while (strcmp(*ptr, "") != 0) {
        if (strcmp(*ptr, "<") == 0 || strcmp(*ptr, ">") == 0 || strcmp(*ptr, "|") == 0) break;
        num++;
        ++ptr;
    } 
    return num;
}

int indexOfChar(char* string, char c) {
    char* p = string;
    for(int i = 0; i < strlen(string); i++) {
        if(p[i] == c) return i;
    }

    return -1;
}

void personalStrCpy(char* destination, char* source, int sourceLength) {
    for(int i = 0; i < sourceLength; i++) {
        destination[i] = source[i];
    }
}

char** concatTokenGlobs(char** tokens, int globIndex, char** globs, int* newBytes) {
    //new number of tokens - 1st dimension
    int numTokens = numberOfTokens(tokens);
    int numGlobs = numberOfTokens(globs);
    int newNumberOfTokens = numTokens + numGlobs - 1; //size of og tokens plus the matches, considering how the original wildcard will also be replaced (-1)
    
    //longest possible string length - 2nd dimension
    int newSize = largestString(tokens);
    if(largestString(globs) > newSize) newSize = largestString(globs);
    //newSize++;    MIGHT NEED TO ADD BACK
    
    //char** new = malloc(sizeof(char*) * newNumberOfTokens); //1st dimension
    char** new = calloc(newNumberOfTokens+3, sizeof(char*));                    //////////////////try changing to +2 instead if needed, need to be this
    *newBytes = newNumberOfTokens+3;
    printf("%d\n", *newBytes);
    for(int i = 0; i < newNumberOfTokens; i++) {
        new[i] = calloc(newSize, sizeof(char));
    }
    
    for(int i = 0; i < globIndex; i++) { //copies og tokens before the matches, stops at the og wildcard
        strcpy(new[i], tokens[i]);
    }
    int globsPtr = 0;
    for(int i = globIndex; i < globIndex+numGlobs; i++) { //replaces the og wildcard and fills in the rest of the matches
        strcpy(new[i], globs[globsPtr]);
        globsPtr++;
    }
    int ogPtr = globIndex + 1;
    for(int i = globIndex+numGlobs; i < newNumberOfTokens; i++) {
        strcpy(new[i], tokens[ogPtr]);
        ogPtr++;
    }
    new[newNumberOfTokens] = ""; new[newNumberOfTokens+1] = ""; new[newNumberOfTokens+2] = "";
    //new[newNumberOfTokens]


    return new;
}


char** wildcard(char** tokens, int globIndex, int ogTokenLength, int* newBytes){
    glob_t globs;
    //printf("%d\n", globIndex);
    int r = glob(tokens[globIndex], GLOB_ERR, NULL, &globs); 
    if (r > 3) {//(r != 0 && r != 1) { //error check
        if(r == GLOB_NOMATCH) 
            fprintf(stderr, "No glob matches\n");
        else
            fprintf(stderr, "Glob Error");
        //exit(0);
    }
    if(globs.gl_pathc > 0) {
        //printf("PASSSS\n");
        globs.gl_pathv[globs.gl_pathc] = "";
        char** eraser = tokens; // might not need
        tokens = concatTokenGlobs(tokens, globIndex, globs.gl_pathv, newBytes);

        globfree(&globs); //free eraser and globs now
        for(int i = 0; i < ogTokenLength; i++) {//numberOfTokens(eraser); i++) {
            free(eraser[i]);
        }
        free(eraser);

        //return new dimensions
    }
    return tokens;
}

int redirectionIn(char* name) {
    int oldfd = dup(STDIN_FILENO);
    //printf("oldfd is %d\n", oldfd);
    int redir = open(name, O_RDONLY);
    dup2(redir, STDIN_FILENO);
    close(redir);
    fflush(stdin);
    return oldfd;
}

int redirectionOut(char* name)
{
    int oldfd = dup(STDOUT_FILENO);
    //printf("oldfd is %d\n", oldfd);
    int redir = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0640);
    dup2(redir, STDOUT_FILENO);
    close(redir);
    return oldfd;
}

/*
void piping(char* pathName, char** argsList) {
    int p[2];
    pipe(p);
    dup2(p[1], 1);
    close(p[1]);

    executeProgram(pathName, argsList);

    dup2(p[0], 0);
    close(p[0]);

}
*/

int executeProgram(char* pathName, char** args) {

    //set up child process,
    int pid = fork();
    if (pid == -1) { //error check
        perror("fork: ");
        return 1;
    }
    if (pid == 0) {
        //exec the arguments
        //printf("we will now execute\n");
        execv(pathName, args);
        perror("");
        //printf("this should never print unless something goes wrong with execv\n");
        return EXIT_FAILURE; //error check
    }
    //checking status of child process
    int waitStatus;
    wait(&waitStatus);   
    if (WIFEXITED(waitStatus)) return EXIT_SUCCESS; //wait error check
    else return EXIT_FAILURE;
}

char* searchName(char** tokenptr) { //searches for a given file name in 1 of 6 directories
    char* fileName = malloc(sizeof(char) * (strlen(*tokenptr) + 2 + 15)); //2 for null terminator and '/', 15 for the longest directory name(/usr/local/sbin)
    memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
    
    struct stat sb;

    strcpy(fileName, "/usr/local/sbin/");
    strcat(fileName, *tokenptr);
        //printf("%s\n", fileName);
    int r = stat(fileName, &sb);
    if (r == -1) { //does not exist in /usr/local/sbin
        memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
        strcpy(fileName, "/usr/local/bin/");
        strcat(fileName, *tokenptr);
            //printf("%s\n", fileName);
        int r = stat(fileName, &sb);
        if (r == -1) { //does not exist in /usr/local/bin
            memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
            strcpy(fileName, "/usr/sbin/");
            strcat(fileName, *tokenptr);
                //printf("%s\n", fileName);
            int r = stat(fileName, &sb);
            if (r == -1) { //does not exist in /usr/sbin
                memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
                strcpy(fileName, "/usr/bin/");
                strcat(fileName, *tokenptr);
                    //printf("%s\n", fileName);
                int r = stat(fileName, &sb);
                if (r == -1) { //does not exist in /usr/bin
                    memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
                    strcpy(fileName, "/sbin/");
                    strcat(fileName, *tokenptr);
                        //printf("%s\n", fileName);
                    int r = stat(fileName, &sb);
                    if (r == -1) { //does not exist in /sbin
                        memset(fileName, (char) 0, (strlen(*tokenptr) + 2 + 15));
                        strcpy(fileName, "/bin/");
                        strcat(fileName, *tokenptr);
                            //printf("%s\n", fileName);
                        int r = stat(fileName, &sb);
                        if (r == -1) { //does not exist in /bin
                            //the described filename cannot be found at all
                            free(fileName);
                            return "";
                        }
                        else return fileName; //return at /bin
                    }
                    else return fileName; //return at /sbin
                }
                else return fileName;//return at /usr/bin
            }
            else return fileName; //return at /usr/sbin
        }
        else return fileName; //return at /usr/local/bin
    }
    else return fileName; //return at /usr/local/sbin

    
    
}

int main(int argc, char** argv) {
    int mode;
    if (argv[1] != NULL) mode = BATCHMODE; //batch mode
    if (argv[1] == NULL) { //interactive mode
        //printf("Welcome to MyShell!\n"); 
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
        printf("We are in batch mode, with a stream file descriptor %d matching to the file \"%s\"\n", sfd, argv[1]);
    }
    
    if (mode == 1) {
        printf("Welcome to MyShell!\n"); 
        printf("mysh> ");
        fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    int bytes = 0;
    while ((bytes = read(sfd, buf, BUFSIZE)) > 0) { //reading in input from stream and writing it to buf
    
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

        if (strcmp(tokens[0], "") == 0) {
            memset(buf, 0, BUFSIZE); //fake flush the buffer
            for (int i = 0; i < bytes; i++) {
                free(tokens[i]);
            }
            free(tokens);
            printf("mysh> ");
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
            looper++; 
        }
            //now look for * in tokens for wildcard replacement//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            int newBytes = 0;
            //printf("");
            for(int i = 0; i < bytes; i++) {
                if(indexOfChar(tokens[i], '*') != -1) {
                    tokens = wildcard(tokens, i, bytes, &newBytes);
                    bytes = newBytes;
                    break;
                }
            }
        
        //TOKEN PRINT TESTING 2
        /*int looper2 = 0; 
        while (strcmp(tokens[looper2], "") != 0) {
            printf("%s\n", tokens[looper2]);
            looper2++;
        }*/

        int shellExit = 0; //tracker for exit command
        /*int redirIn = 0; //tracker for redirections in
        int redirOut = 0; //tracker for redirections out
        int piped = 0; //tracker for pipes (not implementing multiple pipes, only for error checking)
*/
        //begin interpretation of tokens and execution of commands
        char** tokenptr = tokens; //for traversal through tokens
        
        if (strcmp(*tokenptr, "cd") == 0) { //execute cd command
            ++tokenptr;
            /*if (strcmp(*tokenptr, tokens[0]) != 0) { //cd must be the first token in a command according to our implementation
                printf("Inappropriate command! \"cd\" must be at the start of a command.\n");
                tempstatus = 1;
            }*/
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
            }
            else {
                int success = chdir(*tokenptr);
                if (success == -1) { //check if failed
                    perror("");
                    tempstatus = 1;
                }
                else printf("Succesfully changed to specified directory, THIS IS ONLY FOR TESTING REMEMBER TO DELETE THIS\n");
            }
        }
        else if (strcmp(*tokenptr, "pwd") == 0) { //execute pwd command
            /*if (strcmp(*tokenptr, tokens[0]) != 0) { //pwd must be the first token in a command according to our implementation, but its output can be piped/redirected
                perror("Inappropriate command! \"pwd\" must be at the start of a command.\n");
                tempstatus = 1;
            }*/
            char buffer[1000];
            memset(buffer, (char) 0, 1000); 
            char* pathName = getcwd(buffer, 1000);
            //check for redirection or piping first///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            printf("%s\n", pathName);
            ++tokenptr;
        }
        else if (strcmp(*tokenptr, "exit") == 0) { //mark exit status
            shellExit = 1;
            ++tokenptr;
        } 
        else { //first check if the first token contains a /, indicating a path name to an executable program
            char* pathName;

            int j = 0;
            char slash = tokenptr[0][0];
            while (slash != '\0') {
                if (slash == '/') break; 
                j++;
                slash = tokenptr[0][j];
            }
            if (slash == '/') { //found a pathname  
                pathName = tokenptr[0];
                //printf("%s\n", pathName);
                /*int childStatus = executeProgram(pathName, tokens, tokenptr);
                if (childStatus == 1) { //failed to execute
                    printf("Program %s could not be executed\n", pathName);
                    tempstatus = 1;
                }*/
                
            }
            else { //did not find a path name, so search in directories for a file w/ the specified bare name
                pathName = searchName(tokenptr); //DO NOT FORGET TO FREE THE STRING RETURNED BY THIS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if (strcmp(pathName, "") == 0) { //could not find specified bare name
                    printf("Program %s could not be found\n", *tokenptr);
                    tempstatus = 1;
                }
                else {
                    //printf("%s\n", pathName);

                    /*int childStatus = executeProgram(pathName, tokens, tokenptr);
                    if (childStatus == 1) { //failed to execute
                        printf("Program %s could not be executed\n", pathName);
                        tempstatus = 1;
                    }*/
                }
            }

            if (tempstatus != 1) {
                // get arguments for pathName command to execute
                int maxArgs = numberOfArgs(tokens);
                printf("%d\n", maxArgs);
                int largestToken = largestString(tokens);
                int maxSize = strlen(pathName) + 1;

                char** args1 = malloc((maxArgs) * sizeof(char*));
                memset(args1, 0, maxArgs);
                for (int i = 0; i < maxArgs; i++) {
                    args1[i] = malloc(largestToken + maxSize * sizeof(char));
                    memset(args1[i], 0, largestToken+maxSize);

                }

                strcpy(args1[0], pathName);
                //char** tempptr = &(args1[maxArgs]);
                //args1[maxArgs] = (char*) NULL;
                tokenptr++;
                

                int oldinfd = -1;
                int oldoutfd = -1;
                int i = 1;
                while (strcmp(*tokenptr, "") != 0) {
                    if (strcmp(*tokenptr, "<") == 0) {
                        printf("found an input redirection\n");
                        ++tokenptr;
                        oldinfd = redirectionIn(*tokenptr);
                        ++tokenptr;///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        while (strcmp(*tokenptr, "") != 0) {
                            if (strcmp(*tokenptr, "<") == 0) {
                                ++tokenptr;
                                printf("Cannot have multiple input redirections\n");
                                break;
                            }
                            else if (strcmp(*tokenptr, ">") == 0) {
                                ++tokenptr;
                                oldoutfd = redirectionOut(*tokenptr);
                                ++tokenptr;
                            }
                            /*else if (strcmp(*tokenptr, "|") == 0) {
                                
                            }*/
                            else {
                                //printf("THIS IS RIGHT BEFORE THE STRING COPY\n");
                                strcpy(args1[i], *tokenptr);
                                //printf("%s\n", args1[i]);
                                ++tokenptr;
                                i++;
                            }
                        }
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    }
                    else if (strcmp(*tokenptr, ">") == 0) {
                        ++tokenptr;
                        oldoutfd = redirectionOut(*tokenptr);
                        ++tokenptr;
                    }
                    /*else if (strcmp(*tokenptr, "|") == 0) {
                        //check exit in pipe
                    }*/
                    else {
                        //printf("THIS IS RIGHT BEFORE THE STRING COPY\n");
                        if (strcmp(*tokenptr, "") != 0) strcpy(args1[i], *tokenptr);
                        //printf("%s\n", args1[i]);
                        ++tokenptr;
                        i++;
                    }
                }
                

                
                for (int i = 0; i < maxArgs; i++) {
                    printf("Arg1 pointer is %s\n", args1[i]);
                }

                int childStatus = executeProgram(pathName, args1);

                if (oldinfd != -1) {
                    
                    dup2(oldinfd, 0);
                    close(oldinfd);
                }
                if (oldoutfd != -1) {
                    
                    dup2(oldoutfd, 1);
                    close(oldoutfd);
                }

                if (childStatus == 1) { //failed to execute
                    printf("Program %s could not be executed\n", pathName);
                    tempstatus = 1;
                }
                //else printf("program %s should have been executed\n", pathName);

                //args1[maxArgs] = "";
                //args1[maxArgs] = realloc(args1[maxArgs], (largestToken + maxSize) * sizeof(char));
                //strcpy(args1[maxArgs], "");
                /*for (int i = 0; i < largestToken + maxSize-1; i++) {
                    args1[maxArgs][i] = '0';
                }*/
                //args1[maxArgs][largestToken + maxSize-1] = '\0';
                for (int i = 0; i < maxArgs ; i++) {
                    printf("We are freeing args1[%d], which contains %s\n", i, args1[i]);
                    free(args1[i]);
                }
                free(args1);

                if (slash != '/' && strcmp(pathName, "") != 0) {
                    free(pathName);
                }
                //free(tempptr);
            }
        }
        
        memset(buf, 0, BUFSIZE); //fake flush the buffer
        for (int i = 0; i < bytes; i++) {
            free(tokens[i]);
        }
        free(tokens);
        
        if (shellExit == 1) { //check if we need to exit
            printf("exiting\n");
            break;
        }
        else if (mode == INTERACTIVEMODE) {
            if (tempstatus == 1)
            {
                printf("!mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
            else {
                printf("mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            }
        }
        
    
    }
    if (mode == BATCHMODE) close(sfd);
    return EXIT_SUCCESS;
}
