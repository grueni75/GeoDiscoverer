//============================================================================
// Name        : Debug.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

// Path to the message log if used
std::string messageLogPath = "";

// Path to the trace log if used
std::string traceLogPath = "";

namespace GEODISCOVERER {

// Constructor
Debug::Debug() {

  // Init variables
  fatalOccured=false;
  tracelog=NULL;
  messagelog=NULL;
}

// Replays a trace log
void Debug::replayTrace(std::string filename) {

  std::ifstream in;
  std::string line;
  Int x,y;
  double zoom,angle;

  // Time in microseconds to wait before executing the next command during replay
  TimestampInMicroseconds replayPeriod = core->getConfigStore()->getIntValue("General","replayPeriod",__FILE__,__LINE__);

  // Open the file
  in.open (filename.c_str());
  if (!in.is_open()) {
    return;
  }

  // Go through it line by line
  while(!in.eof()) {
    getline(in,line);

    // Extract the command
    std::stringstream strm(line);
    std::string delim,cmd;
    strm >> delim >> delim >> cmd;
    cmd=cmd.substr(0,cmd.size()-1);

    // And execute it
    if (cmd!="createGraphic()") {
      if (!core->getIsInitialized())
        return;
      core->getCommander()->execute(cmd);
    }

    // Sleep a little bit
    usleep(replayPeriod);
    //sleep(2);
    /*char buffer[32];
    gets(buffer);*/

  }
  in.close();
}

// Opens the necessary files
void Debug::init() {

  // Set the log directory
  std::string logPath=core->getHomePath() + "/Log";
  std::string timestamp = core->getClock()->getFormattedDate();

  // Check if the log directory exists
  struct stat st;
  if (stat(logPath.c_str(), &st) != 0)
  {
    if (mkdir(logPath.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      puts("FATAL: can not create log directory!");
      exit(1);
    }
  }

  // Open the trace log
  if (core->getConfigStore()->getIntValue("General","createTraceLog", __FILE__, __LINE__)) {
    std::string tracelog_filename = logPath + "/trace-" + timestamp  + ".log";
    traceLogPath = tracelog_filename + "\n";
    //std::string tracelog_filename = logPath + "/trace-" + "" + ".log";
    if (!(tracelog=fopen(tracelog_filename.c_str(),"w"))) {
      puts("FATAL: can not open trace log for writing!");
      exit(1);
    }
  }

  // Open the message log
  if (core->getConfigStore()->getIntValue("General","createMessageLog", __FILE__, __LINE__)) {
    std::string messagelog_filename = logPath + "/message-" + timestamp + ".log";
    messageLogPath = messagelog_filename + "\n";
    //std::string tracelog_filename = logPath + "/trace-" + "" + ".log";
    if (!(messagelog=fopen(messagelog_filename.c_str(),"w"))) {
      puts("FATAL: can not open message log for writing!");
      exit(1);
    }
  }
}


// Writes a message to a file
void Debug::write(FILE *out, const char *prefix, const char *postfix, const char *relative_file, int line, const char *timestamp, const char *fmt, va_list argp) {
  if (out) {
    fprintf(out,"%-7s: ",prefix);
    vfprintf(out,fmt, argp);
    fprintf(out,"%s [%s:%d,%s]\n",postfix,relative_file,line,timestamp);
  }
}

// Destructor
Debug::~Debug() {

  // Close the log files
  if (tracelog)
    fclose(tracelog);
  if (messagelog)
    fclose(messagelog);
}

}
