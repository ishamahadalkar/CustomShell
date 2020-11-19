#include <cstdio>

#include "shell.hh"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <cstring>
#include <fstream>
#include <algorithm>

void myunputc(int);

int yyparse(void);
std::string absolute_path;

// extern bool on_error;
extern bool on_error;

// External function for sending shell data
std::string shell_exec() {
  return absolute_path;
}

// External function for handling Ctrl C
extern "C" void sigHandler_CtrlC(int sig) {
  if (sig == SIGINT) {
    fprintf(stderr, "\n");

    // If we dont have any processes running only then print the prompt
    if (Shell::_currentCommand._simpleCommandsArray.size() < 1) {
      Shell::prompt();
    }    
  }
}


// External function for handling zombie processes
extern "C" void sigHandler_zombie(int sig) {
  if (sig == SIGCHLD) {
    int wstatus;
    waitpid(-1, &wstatus, 0);
  
    while (waitpid((pid_t) -1, 0, WNOHANG) > 0) {
      //fprintf(stdout, "[%d] exited.\n", PID);
      Shell::prompt();
    }
  }
}

void Shell::prompt() {

  // Checking for PROMPT environemnt variable
  char * prompt_env = getenv("PROMPT");
  char * onError_env = getenv("ON_ERROR");

  // fprintf(stdout, "err: %d\n", _currentCommand._onerror);

  if (onError_env == NULL) {
    _currentCommand._onerror = -1;
  }
  
  // When PROMPT is not available
  if (isatty(0) && prompt_env == NULL) {
    printf("myshell>");
  }

  // When PROMPT is availabe and ON_ERROR is false
  if (isatty(0) && prompt_env != NULL && _currentCommand._onerror  != 0) {
    printf("%s", prompt_env);
  }

  // When ON_ERROR is true
  if (isatty(0) && _currentCommand._onerror == 0) {
    printf("%s", onError_env);
  }

  _currentCommand._onerror = -1;
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  
  // Checking for Ctrl C
  struct sigaction signalAction;
  signalAction.sa_handler = sigHandler_CtrlC;
  sigemptyset(&signalAction.sa_mask);
  signalAction.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &signalAction, NULL)) {
      perror("sigaction ctrl c");
      exit(2);
  }


  // Checking for Zombie Processes
  struct sigaction signalZombie;
  signalZombie.sa_handler = sigHandler_zombie;
  sigemptyset(&signalZombie.sa_mask);
  signalZombie.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &signalZombie, NULL)) {
      perror("sigaction zombie");
      exit(2);
  }

  // Finding the name of the shell executable
  char * file = argv[0];
  std::string xr(file);
  absolute_path = file;

  // Implementing .shellrc
  // C++ string to store the line and the ifstream to read from file
  // std::string line;
  // std::string reversed;
  // std::ifstream file_rc(".shellrc");

  // while (std::getline(file_rc, line)) {
  //     // Reversing the C++ string myunputc
  //     reversed = line;
  //     std::reverse(reversed.begin(), reversed.end());

  //     // Add a new line character since we are technically starting from the end
  //     myunputc('\n');
  //     for (unsigned int k = 0; k < reversed.size(); k++) {
  //       myunputc(reversed[k]);
  //     }

  // } // end while

  Shell::prompt();
  yyparse();

}

Command Shell::_currentCommand;
