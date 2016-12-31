//============================================================================
// Name        : Debug.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
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
// History:
//
// $Log: Debug.h,v $
// Revision 1.12  2011/09/05 05:59:35  grueni
// profiling
//
// Revision 1.11  2011/01/19 07:25:34  grueni
// gpx file creation continued
//
// Revision 1.10  2010/10/28 06:00:33  grueni
// debugged command dispatching of button
// removed not required virtuals
//
// Revision 1.9  2010/09/15 08:23:30  grueni
// First trials on real device
// Introduced map update thread for smoother drawing
//
// Revision 1.8  2010/09/06 08:15:13  grueni
// progress dialog support started
// main loop put into a thread
//
// Revision 1.7  2010/09/04 20:39:58  grueni
// jni handling implemented
// sd card monitoring implemented
// debug support implementation started
//
// Revision 1.6  2010/08/25 08:11:32  grueni
// components moved in single object
// restructured
//
// Revision 1.5  2010/08/11 05:38:50  grueni
// trace/replay function added
// debugging
//
// Revision 1.4  2010/05/02 15:24:44  grueni
// directory reading debugging
//
// Revision 1.3  2010/01/01 17:54:30  grueni
// config class implemented
// position handling screen class implemented
//
// Revision 1.2  2009/12/31 15:14:30  grueni
// checker board and keyboard control implemented
//
// Revision 1.1  2009/12/30 22:20:47  grueni
// code style adapted to K&R
//
//
//============================================================================


#ifndef DEBUG_H_
#define DEBUG_H_

namespace GEODISCOVERER {

// Verbosity levels
enum Verbosity { verbosityError=0, verbosityWarning=1, verbosityInfo=2, verbosityDebug=3, verbosityFatal=4, verbosityTrace=5 };

// Message macros
#define DEBUG(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityDebug,__FILE__,__LINE__,false,msg,__VA_ARGS__)
#define INFO(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityInfo,__FILE__,__LINE__,false,msg,__VA_ARGS__)
#define WARNING(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityWarning,__FILE__,__LINE__,false,msg,__VA_ARGS__)
#define ERROR(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityError,__FILE__,__LINE__,false,msg,__VA_ARGS__)
#define FATAL(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityFatal,__FILE__,__LINE__,false,msg,__VA_ARGS__)
#define TRACE(msg, ...) if (core->getDebug()) core->getDebug()->print(verbosityTrace,__FILE__,__LINE__,false,msg,__VA_ARGS__)

// Callstack macro
#define CALLSTACK(result) \
{ \
  result=""; \
  char **strings; \
  void *array[10]; \
  size_t size; \
  size = backtrace (array, 10); \
  strings = backtrace_symbols (array, size); \
  for (int i = 0; i < size; i++) { \
    result += std::string(strings[i]) + "\n"; \
  } \
}

// Debug class
class Debug {

protected:

  // Handle to the trace log file
  FILE *tracelog;

  // Handle to the message log file
  FILE *messagelog;

  // Indicates if a fatal message has occured
  bool fatalOccured;

  // Indicates if a trace log file shall be created
  bool createTraceFile;

public:

  // Constructor
  Debug();

  // Opens the necessary files
  void init();

  // Prints out a message
  void print(Verbosity verbosity, const char *file, int line, bool messageLogOnly, const char *fmt, ...);

  // Writes a message to a file
  void write(FILE *out, const char *prefix, const char *postfix, const char *relative_file, int line, const char *timestamp, const char *fmt, va_list argp);

  // Replays a trace log
  void replayTrace(std::string filename);

  // Destructor
  virtual ~Debug();

  // Getters and setters
  bool getFatalOccured() const
  {
      return fatalOccured;
  }

};

}

#endif /* DEBUG_H_ */
