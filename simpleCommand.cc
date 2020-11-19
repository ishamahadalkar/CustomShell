#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>
#include <string.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <functional>

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>

#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include "simpleCommand.hh"

// Vector of strings to store the arguments which match the regex
std::vector<std::string> args;

SimpleCommand::SimpleCommand() {
  _argumentsArray = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  for (auto & arg : _argumentsArray) {
    delete arg;
  }
}

void SimpleCommand::insertArgument( std::string * argument ) {
  _argumentsArray.push_back(argument);
}


void SimpleCommand::expandWildcards(char * prefix, char * suffix) {
  if (suffix[0] == '\0') {
    // Suffix is empty so we put prefix in as a argument
    prefix++;
    args.push_back(prefix);
    return;
  }

  // Obtain the next component in the suffix
  // Also advance the suffix
  char * s = strchr(suffix, '/');
  char component[1089];
  
  // Copy the string upto the first '/'
  if (s != NULL) {
    if (s - suffix != 0) {
      strncpy(component, suffix, s - suffix);
    }
    else {
      strcpy(component, "");
    }
    suffix = s + 1;
  }
  else {
    // It's the last part of the path.
    // Copy the whole string
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  // Now we expand the component
  char newPrefix[1089];
  if (!(strchr(component, '*') || strchr(component, '?'))) {
    // Component does not have wildcards
    sprintf(newPrefix, "%s/%s", prefix, component);
    expandWildcards(newPrefix, suffix);
    return;
  }

  // If component has wildcards
  // Converting Component to Regular Expression
  // Converting Wildcard to Regular Expression
  // Convert “*” ­> “.*”
  // “?” ­> “.”
  // “.” ­> “\.”

  char r[1089];
  
  unsigned int i = 0; // Index for text
  unsigned int j = 0; // Index for regex i.e r
  // Match beginning of the line
  r[j] = '^';
  j++;
  while (i < strlen(component)) {
    if (component[i] == '*') {
      r[j] ='.';
      j++;
      r[j] = '*';
      j++;
    }
    else if (component[i] == '?') {
      r[j] = '.';
      j++;
    }
    else if (component[i] == '.') {
      r[j] = '\\';
      j++;
      r[j] = '.';
      j++;
    }
    else {
      r[j] = component[i];
      j++;
    }
    i++;
  } // end while

  // Matching the end of line and adding NULL character
  r[j] = '$';
  j++;
  r[j] = '\0';

  // Compiling the regular expression
  regex_t re;
    int result = regcomp( &re, r,  REG_EXTENDED);
    if (result != 0) {
        perror("bad sub wildcard regex");
        exit(-1);
    }

  // Listing directories and adding as arguments the entires that match
  // the regular expression

  const char * dir;
  // If prefix is empty then list the current directory

  if (prefix[0] == '\0') {
    dir = ".";
  }
  else {
    dir = prefix;
  }
    
  DIR * d = opendir(dir);
  if (d == NULL) {
    // Freeing the regex
    // delete r;
    regfree(&re);
    return;
  }

  // Now we check what entries match
  struct dirent * ent;
  while ((ent = readdir(d)) != NULL) {
    // Check if name matches
    regmatch_t match;
    result = regexec(&re, ent->d_name, 1, &match, 0);
    if (result == 0) {
      // Entry matches
      // Adding name of entry that matches to the prefix and call expandWildacards recursively

      // Checking for the first '.' or '..'
      
      if (ent->d_name[0] == '.') {
        if (component[0] == '.') {
          sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
          expandWildcards(newPrefix, suffix);
        }
      }
      else {
        // Add argument
          sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
          expandWildcards(newPrefix, suffix);
      }
    }

  }

  // If there are no matches with the pattern
  if (suffix[0] == '\0' && prefix[0] != '\0' && args.size() == 0) {
    char temp_insert[1089];
    prefix++;
    strcpy(temp_insert, prefix);
    const char * x = "/";
    strcat(temp_insert, x);
    strcat(temp_insert, component);
    args.push_back(temp_insert);

    // Freeing the regex
    // delete r;
    regfree(&re);
    return;
  }

  closedir(d);

  // Freeing the regex
  // delete r;
  regfree(&re);
}

void SimpleCommand::expandWildcardsIfNecessary( std::string * argument ) {
  // Finding the string
  const char * text = argument->c_str();

  // // Vector of strings to store the arguments which match the regex
  // std::vector<std::string> args;

  // Return if text does not contain “*” or “?”

  if (!(strchr(text, '*') || strchr(text, '?'))) {
    insertArgument(argument);
    return;
  }

  // Special case for handling the variable expansion of ${?}
  if (strcmp(text, "${?}") == 0) {
    insertArgument(argument);
    return;
  }

  char * prefix = (char *) "";
  char * suffix = (char *) argument->c_str();
  expandWildcards(prefix, suffix);

  // Sorting the args array
  std::sort(args.begin(), args.end());

  // Adding the arguments
  for (unsigned int i = 0; i < args.size(); i++) {
    //std::string * data = new std::string(args[i]);
    _argumentsArray.push_back(new std::string(args[i]));
  }

    // Remove all references to the arguments we've deallocated
    args.clear();
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _argumentsArray) {
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
