#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

//extern bool on_error;

// Command Data Structure

struct Command {
  // Array of simple commands
  std::vector<SimpleCommand *> _simpleCommandsArray;
  std::string * _outFileName;
  std::string * _inFileName;
  std::string * _errFileName;
  std::string _lastargument;
  bool _backgnd;
  bool _append;
  int _redirects;
  int _onerror;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  static Command *_currCommand;
  static SimpleCommand *_currSimpleCommand;
};

#endif
