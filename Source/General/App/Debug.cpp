//============================================================================
// Name        : Debug.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// GeoDiscoverer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GeoDiscoverer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GeoDiscoverer.  If not, see <http://www.gnu.org/licenses/>.
//
//============================================================================

#include <Core.h>

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
    if (cmd!="createGraphic()")
      core->getCommander()->execute(cmd);

    // Sleep a little bit
    //usleep(10000);

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
  if (core->getConfigStore()->getIntValue("General","createTraceLog")) {
    std::string tracelog_filename = logPath + "/trace-" + timestamp  + ".log";
    //std::string tracelog_filename = logPath + "/trace-" + "" + ".log";
    if (!(tracelog=fopen(tracelog_filename.c_str(),"w"))) {
      puts("FATAL: can not open trace log for writing!");
      exit(1);
    }
  }

  // Open the message log
  if (core->getConfigStore()->getIntValue("General","createMessageLog")) {
    std::string messagelog_filename = logPath + "/message-" + timestamp + ".log";
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
