#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE 1000
#define BATCHMODE 0
#define INTERACTIVEMODE 1


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
        fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////
    }

    int bytes = 0;
    while ((bytes = read(sfd, buf, BUFSIZE)) > 0) { //reading in input from stream and writing it to buf
        if(mode == INTERACTIVEMODE) {

            /*for (int i = 0; i < bytes; i++) {
                printf("%c", buf[i]);
            }
            */
            char** tokens = malloc(sizeof(char*) * bytes);
            memset(tokens, (char) 0, bytes);
            for (int i = 0; i < bytes; i++) {
                tokens[i] = malloc(sizeof(char) * bytes);
                memset(tokens[i], (char) 0, bytes);
            }

            int valid = tokenize(buf, tokens); //parse command input inside buffer as an array of tokens
            if (valid == 1) { //check for valid command syntax
                perror("Inappropriate command syntax!\n");
                memset(buf, (char) 0, BUFSIZE); //fake flush the buffer
                for (int i = 0; i < bytes; i++) {
                    free(tokens[i]);
                }
                free(tokens);
                printf("mysh> ");
                fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////
                continue;
            }
            
            //TOKEN PRINT TESTING
            /*int looper = 0; 
            while (strcmp(tokens[looper], "") != 0) {
                printf("%s\n", tokens[looper]);
                looper++;
            }*/

            int shellExit = 0; //tracker for exit command

            //begin interpretation of tokens and execution of commands
            char** tokenptr = tokens; //for traversal through tokens
            while (strcmp(*tokenptr, "") != 0) {
                if (strcmp(*tokenptr, "cd") == 0) { //execute cd command
                    if (strcmp(*tokenptr, tokens[0]) != 0) { //cd must be the first token in a command according to our implementation
                        perror("Inappropriate command! \"cd\" must be at the start of a command.\n");
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
                        }
                        else printf("Succesfully changed to home directory, THIS IS ONLY FOR TESTING REMEMBER TO DELETE THIS\n");
                        break;
                    }
                    else {
                        int success = chdir(*tokenptr);
                        if (success == -1) { //check if failed
                            perror("");
                        }
                        else printf("Succesfully changed to specified directory, THIS IS ONLY FOR TESTING REMEMBER TO DELETE THIS\n");
                        break;
                    }
                }
                else if (strcmp(*tokenptr, "pwd") == 0) { //execute pwd command
                    if (strcmp(*tokenptr, tokens[0]) != 0) { //pwd must be the first token in a command according to our implementation, but its output can be piped/redirected
                        perror("Inappropriate command! \"pwd\" must be at the start of a command.\n");
                        break;
                    }
                    char buffer[1000];
                    memset(buffer, (char) 0, 1000); 
                    char* pathName = getcwd(buffer, 1000);
                    //check for redirection or piping first///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    printf("%s\n", pathName);
                    ++tokenptr;
                }
                else if (strcmp(*tokenptr, "exit") == 0) { //execute exit command
                    shellExit = 1;
                } 
                /*else { //first check if the first token contains a /, indicating a path name to an executable program
                    int j = 0;
                    char slash = *tokenptr[0];
                    int pathFound = 0;
                    while (slash != NULL) {
                        if (slash == '/') {
                            pathFound = 1; //found a pathname   
                            break;
                        }
                        j++;
                        slash = *tokenptr[j];
                    }
                    if (slash == '/') { //now execute program at pathname 

                    }
                    else { //did not find a path name, so search in directories for a file w/ the specified bare name
                        
                    }
                }
                */
               ++tokenptr;
            }

            memset(buf, 0, BUFSIZE); //fake flush the buffer
            for (int i = 0; i < bytes; i++) {
                free(tokens[i]);
            }
            free(tokens);
            printf("mysh> ");
            fflush(stdout); //be ready to change this to a different output if needed?/////////////////////////////////////////
            if (shellExit == 1) { //check if we need to exit
                printf("exiting\n");
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}
