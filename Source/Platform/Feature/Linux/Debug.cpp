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

namespace GEODISCOVERER {

// Prints a message
void Debug::print(Verbosity verbosity, const char *file, int line, bool messageLogOnly, const char *fmt, ...) {

  va_list argp;
  const char *prefix,*postfix;
  const char *relative_file;
  FILE *out=stdout;

  // Remove the trailing source path from the file
  relative_file=strstr(file,SRC_ROOT);
  if (!relative_file)
    relative_file=file;
  else
    relative_file=file+strlen(SRC_ROOT)+1;

  switch(verbosity) {
  case verbosityFatal:
    prefix="FATAL";
    postfix="!";
    fatalOccured=true;
    break;
  case verbosityError:
    prefix="ERROR";
    postfix="!";
    break;
  case verbosityWarning:
    prefix="WARNING";
    postfix="!";
    break;
  case verbosityInfo:
    prefix="INFO";
    postfix=".";
    break;
  case verbosityDebug:
    prefix="DEBUG";
    postfix=".";
    break;
  case verbosityTrace:
    out=tracelog;
    prefix="TRACE";
    postfix=".";
    break;
  default:
    prefix="UNKNOWN";
    postfix="!";
    break;
  }
  std::string timestamp=core->getClock()->getXMLDate().c_str();
  va_start(argp, fmt);
  write(out,prefix,postfix,relative_file,line,timestamp.c_str(),fmt,argp);
  va_end(argp);
  if (out==stdout) {
    va_start(argp, fmt);
    write(messagelog,prefix,postfix,relative_file,line,timestamp.c_str(),fmt,argp);
    va_end(argp);
    fflush(messagelog);
  }
  if (verbosity==verbosityTrace)
    fflush(out);
  if (verbosity==verbosityFatal)
    exit(1);
}

}
