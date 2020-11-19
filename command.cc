/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <string>
#include <string.h>
#include <iostream>
#include <fstream>

#include "command.hh"
#include "shell.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <algorithm>
#include <regex.h>
#include <dirent.h>

#include <cstring>
#include "y.tab.hh"

void myunputc(int);
extern std::string shell_exec();
// extern std::string get_last_argument();

// Declaring the environ variable
extern char ** environ;

// Dynamically allocated memory
std::string * act;
char * actual = new char[1089];
regex_t re;

Command::Command() {

    // Initialize a new vector of Simple Commands
    _simpleCommandsArray = std::vector<SimpleCommand *>();

    _outFileName = NULL;
    _inFileName = NULL;
    _errFileName = NULL;
    _backgnd = false;
    _append = false;
    _redirects = 0;
    _onerror = -1;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommandsArray.push_back(simpleCommand);
}

void Command::clear() {

    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommandsArray) {
        // Delete the argument vector 
        delete simpleCommand;
    }
    

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommandsArray.clear();

    if ( _outFileName ) {
        delete _outFileName;
    }
    _outFileName = NULL;

    if ( _inFileName ) {
        delete _inFileName;
    }
    _inFileName = NULL;

    if ( _errFileName ) {
        delete _errFileName;
    }
    _errFileName = NULL;

    _backgnd = false;
    _append = false;
    _redirects = 0;

    // delete act;
} 


void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommandsArray ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFileName?_outFileName->c_str():"default",
            _inFileName?_inFileName->c_str():"default",
            _errFileName?_errFileName->c_str():"default",
            _backgnd?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommandsArray.size() == 0 ) {
        Shell::prompt();
        return;
    }

    // Find the first simple command"s first argument being entered
    
    const char * firstCommand = _simpleCommandsArray[0]->_argumentsArray[0]->c_str();
    if (strcmp(firstCommand, "exit") == 0) {
        fprintf(stdout, "Good Bye!!\n");
        clear();
        exit(0);
    }

    // If Output Redirection is ambiguos print out error message
    if ( _redirects > 1 ) {
        fprintf(stdout, "Ambiguous output redirect.\n");
        clear();
        Shell::prompt();
    }

    // Print contents of Command data structure
    // print();

    // Add execution here

    // Saving stdin, stdout an stderr
    int tempin = dup(0);
    int tempout = dup(1);
    int temperr = dup(2);

    // Setting the initial input
    // Reading from file
    int fdin;
    if (_inFileName) {
        fdin = open(_inFileName->c_str(), O_RDONLY);
    }
    else {
        // Use default input
        fdin = dup(tempin);
    }

    // Setting the stderr
    int fderr;
    if (_errFileName) {
        // Check if we need to append to file
        if (_append) {
            fderr = open(_errFileName->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0664);
        }
        else {
            fderr = open(_errFileName->c_str(), O_WRONLY | O_TRUNC | O_CREAT , 0664);
        }
    }
    else {
        // Use default output
        fderr = dup(temperr);
    }

    // Redirect error
    dup2(fderr, 2);
    close(fderr);

    int ret;
    int fdout;
    int _numOfSimpleCommands = _simpleCommandsArray.size();

    // Declaring variables for Environment Variable Expansion
    int _lastexit;
    int _lastpid = 0;

    char var[1089];
    char get[1089];
    char begin[1089];
    char end[1089];
    bool ret_b = false;

    char regex[1089] = "^.*[${][^}][^}]*[}].*$";
    // First we compile the regex to use for regexec
    // regex_t re;
    int result = regcomp( &re, regex,  REG_EXTENDED);
    if (result != 0) {
        perror("bad regex");
        regfree(&re);
        exit(-1);
    }

    for (int i = 0; i < _numOfSimpleCommands; i++) {
        // // Redirect input
        // dup2(fdin, 0);
        // close(fdin);

        // Performing variable expansion for each argument in the simple commands array
        std::string * argument;
        for (unsigned int j = 0; j < _simpleCommandsArray[i]->_argumentsArray.size(); j++) {
            // Finding the string
            char * text = (char *) _simpleCommandsArray[i]->_argumentsArray[j]->c_str();
            argument = _simpleCommandsArray[i]->_argumentsArray[j];

            // Now we actually match it
            regmatch_t match;
            result = regexec( &re, text, 1, &match, 0 );

            if (result != 0) {
                // There was no match so just simply add the argument to the array

                // Checking for Tilde expansion
                if ((text[0] == '~' ) || ((text[0] == '~') && (text[1] == '/'))) {
                    if (strlen(text) == 1) {
                        char * home_directory = getenv("HOME");
                        strcpy(actual, home_directory);
                    }
                    else {
                        text = (char *) (text + 1);
                        const char * directory = "/homes/";
                        strcpy(actual, directory);
                        strcat(actual, text);
                    }
                }
                else {
                    // Just add the argument
                    _simpleCommandsArray[i]->_argumentsArray[j] = argument;
                    continue;
                }
                // Then add the result to the vector
                std::string actual_string(actual);
                *argument = actual_string;
                _simpleCommandsArray[i]->_argumentsArray[j] = argument;
                continue;
            }
            
            unsigned int x = 0;
            int y = 0;
            int k;
            unsigned int start_index;
            unsigned int end_index;
        
            strcpy(begin, text);
            // char * end;
            while (strchr(text, '$') && strchr(text, '{')) {
                while (x < strlen(text)) {
                    if (text[x] == '$' && text[x + 1] == '{') {
                        start_index = x;
                        // Clipping the begin string
                        begin[start_index] = '\0';
                        y = x + 2;
                        k = 0;
                        while (text[y] != '}') {
                            var[k] = text[y];
                            k++;
                            y++;
                        }
                        // Setting the null terminator
                        var[k] = '\0';
                        end_index = y + 1;
                        strcpy(end, text + end_index);

                        // var contains the actual expansion text that is passed in the string
                        // eg. aaaa${var}bbb
                        
                        // Finding the actual variable
                        if (strcmp(var, "$") == 0) {
                        // The PID of the shell process
                        int pid = getpid();
                        sprintf(get, "%d", pid);
                        }
                        else if (strcmp(var, "?") == 0) {
                            // The return code of the last executed simple command
                            sprintf(get, "%d", _lastexit);
                        }
                        else if (strcmp(var, "!") == 0) {
                            // PID of the last process run in the background
                            sprintf(get, "%d", _lastpid);
                        }
                        else if (strcmp(var, "_") == 0){
                            // The last argument in the fully expanded previous command
                            strcpy(get, _lastargument.c_str());
                        }
                        else if (strcmp(var, "SHELL") == 0) {
                            // The path of your shell executable.
                            std::string sh = shell_exec();
                            std::string x;
                            // const char * file = "./shell";
                            x = realpath(sh.c_str(), NULL);
                            act = new std::string(x.c_str());
                        }
                        else {
                            // Finding the env variable from var
                            strcpy(get, getenv(var));
                        }
        
                        // Building the new string
                        actual = begin;
                        if (get[0] != '\0') {
                            strcat(actual , get);
                        }
                        else {
                            _simpleCommandsArray[i]->_argumentsArray[j] = new std::string(act->c_str());
                            // delete act;
                            ret_b = true;
                        }
                        if (end != NULL) {
                            strcat(actual , end);
                        }
                        break;
                    }
                    x++;
                } // End first while

                // Special case
                if (ret_b == true) {
                    break;
                }

                // Setting text to the actual string
                x = 0;
                strcpy(text, actual);
            } // End second while

            // Special case
            if (ret_b == true) {
                break;
            }

            // Then add the result to the vector
            std::string actual_string(actual);
            *argument = actual_string;
            _simpleCommandsArray[i]->_argumentsArray[j] = argument;

        }

        // Implementing the BUILT_IN Commands
        // Finding the first argument of the simple command
        const char * command = _simpleCommandsArray[i]->_argumentsArray[0]->c_str();

        // Checking for "setenv"
        if (strcmp(command, "setenv") == 0) {
            const char * var = _simpleCommandsArray[i]->_argumentsArray[1]->c_str();
            const char * value = _simpleCommandsArray[i]->_argumentsArray[2]->c_str();

            int result = setenv(var, value, 1);
            if (result == - 1) {
                perror("error in setenv");
            }
            clear();

            // New
            // Restore in/out/err defaults
            dup2(tempin, 0);
            dup2(tempout, 1);
            dup2(temperr, 2);
            
            close(tempin);
            close(tempout);
            close(temperr);

            Shell::prompt();
            regfree(&re);

            // Closing File Descriptor
            dup2(fdin, 0);
            close(fdin);
            return;
        }

        // Checking for "unsetenv"
        if (strcmp(command, "unsetenv") == 0) {
            const char * var = _simpleCommandsArray[i]->_argumentsArray[1]->c_str();

            int result = unsetenv(var);
            if (result == - 1) {
                perror("error in unsetenv");
            }
            clear();
            // New
            // Restore in/out/err defaults
            dup2(tempin, 0);
            dup2(tempout, 1);
            dup2(temperr, 2);
            
            close(tempin);
            close(tempout);
            close(temperr);

            Shell::prompt();
            regfree(&re);

            // Closing File Descriptor
            dup2(fdin, 0);
            close(fdin);

            return;
        }

        // Checking for "source"
        if (strcmp(command, "source") == 0) {
            const char * var = _simpleCommandsArray[i]->_argumentsArray[1]->c_str();

            // C++ string to store the line and the ifstream to read from file
            std::string line;
            std::string reversed;
            std::ifstream file(var);

            while (std::getline(file, line)) {
                // Reversing the C++ string myunputc
                reversed = line;
                std::reverse(reversed.begin(), reversed.end());

                // Add a new line character since we are technically starting from the end
                myunputc('\n');
                for (unsigned int k = 0; k < reversed.size(); k++) {
                    myunputc(reversed[k]);
                }

            } // end while
       
            clear();

            // New
            // Restore in/out/err defaults
            dup2(tempin, 0);
            dup2(tempout, 1);
            dup2(temperr, 2);
            
            close(tempin);
            close(tempout);
            close(temperr);

            regfree(&re);

            // Closing File Descriptor
            dup2(fdin, 0);
            close(fdin);

            return;

        } // end source

        // Checking for "cd"
        if (strcmp(command, "cd") == 0) {

            int result;
            const char * path;

            if (_simpleCommandsArray[i]->_argumentsArray.size() == 1) {
                char * home_directory = getenv("HOME");
                if (home_directory == NULL) {
                    home_directory = getpwuid(getuid())->pw_dir;
                }
                result = chdir(home_directory);
            }
            else {
                // Go to specified directory
                path = _simpleCommandsArray[i]->_argumentsArray[1]->c_str();
                result = chdir(path);
            }
            if (result == - 1) {
                char err_text[89] = "cd: can't cd to ";
                strcat(err_text, path);
                perror(err_text);
            }

            // New
            // Restore in/out/err defaults
            dup2(tempin, 0);
            dup2(tempout, 1);
            dup2(temperr, 2);
            
            close(tempin);
            close(tempout);
            close(temperr);

            clear();
            Shell::prompt();

            regfree(&re);

            // Closing File Descriptor
            dup2(fdin, 0);
            close(fdin);

            return;

        } // end cd

        // Redirect input
        dup2(fdin, 0);
        close(fdin);


        // Setup output
        if (i == _numOfSimpleCommands - 1) {
            // If it is the last simple command i.e we dont need to pipe it
            // Writing to file
            if (_outFileName) {
                // Check if we need to append to file
                if (_append) {
                    fdout = open(_outFileName->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0664);
                }
                else {
                    fdout = open(_outFileName->c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0664);
                }
                
            }
            else {
                // Use default output
                fdout = dup(tempout);
            }
        }
        else {
            // It's not the last simple command so we need to create a pipe
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1];
            fdin = fdpipe[0];
        }

        // Redirect output
        dup2(fdout, 1);
        close(fdout);

        // For every simple command fork a new process
        ret = fork();

        // Converting the _simpleCommandsArray vector to a char **

        // Finding the number of arguments
        unsigned int _numOfArgs = _simpleCommandsArray[i]->_argumentsArray.size();

        // Converting the string vector to a char * vector
        std::vector<char *> _argCharArray(_numOfArgs + 1);

        // Add the arguments to the char **
        unsigned int j;
        for (j = 0; j < _numOfArgs; j++) {
            _argCharArray[j] = (char *) _simpleCommandsArray[i]->_argumentsArray[j]->c_str();
        }
        // Setting the entry after last argument to NULL for execvp()
        _argCharArray[j] = NULL;
        

        // In the child process
        if (ret == 0) {
            // Implementing Built-In function "printenv"

            // Checking if the first argument in the simple command is "printenv"
            if (strcmp(_argCharArray[0], "printenv") == 0) {
                char ** temp = environ;

                int i = 0;
                while (temp[i] != NULL) {
                    printf("%s\n", temp[i]);
                    i++;
                }

                // Then we exit
                fflush(stdout);
                // _onerror = true;
                
                _exit(1);
            }

            // Load the Program
            execvp(_argCharArray[0], _argCharArray.data());
            perror("execvp in child process");
            _onerror = true;
            _exit(1);
        }
        // If theres an error
        if (ret < 0) {
          perror("error in fork");
        }

        // Parent shell continues

    } // End For Loop

    // Restore in/out/err defaults
    dup2(tempin, 0);
    dup2(tempout, 1);
    dup2(temperr, 2);
    
    close(tempin);
    close(tempout);
    close(temperr);


    if (!_backgnd) {
        // Wait for the last process
        waitpid(ret, &_lastexit, 0);

        // Setting the last exit code variable
        _lastexit = WEXITSTATUS(_lastexit);
        if (_lastexit != 0) {
            _onerror = 0;
        }
    }
    else {
        _lastpid = ret;
    }
    // Setup i/o redirection
    // and call
    
    // Clear to prepare for next command
    clear();   

    // Print new prompt
    Shell::prompt();
    
} // end execute

SimpleCommand * Command::_currSimpleCommand;
