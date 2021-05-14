//============================================================================
// Name        : Debug.cpp
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
//============================================================================

#include <Core.h>
#include <Debug.h>
#include <android/log.h>

// Executes an command on the java side
std::string GDApp_executeAppCommand(std::string command);

// Handles a fatal situation
void GDApp_handleFatal();

// Adds a message on the java side
void GDApp_addMessage(int severity, std::string tag, std::string message);

namespace GEODISCOVERER {

// Prints a message
void Debug::print(Verbosity verbosity, const char *file, int line, bool messageLogOnly, const char *fmt, ...) {

  va_list argp;
  const char *prefix,*postfix;
  const char *relative_file;
  FILE *out=stdout;
  int severity;
  const int buffer_len=256;
  char buffer[buffer_len],buffer2[buffer_len];
  android_LogPriority logPrio;

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
    logPrio=ANDROID_LOG_FATAL;
    break;
  case verbosityError:
    prefix="ERROR";
    postfix="!";
    logPrio=ANDROID_LOG_ERROR;
    break;
  case verbosityWarning:
    prefix="WARNING";
    postfix="!";
    logPrio=ANDROID_LOG_WARN;
    break;
  case verbosityInfo:
    prefix="INFO";
    postfix=".";
    logPrio=ANDROID_LOG_INFO;
    break;
  case verbosityDebug:
    prefix="DEBUG";
    postfix=".";
    logPrio=ANDROID_LOG_DEBUG;
    break;
  case verbosityTrace:
    out=tracelog;
    prefix="TRACE";
    postfix=".";
    break;
  default:
    prefix="UNKNOWN";
    postfix="!";
    verbosity=verbosityError;
    logPrio=ANDROID_LOG_UNKNOWN;
    break;
  }
  va_start(argp, fmt);
  vsnprintf(buffer,buffer_len,fmt,argp);
  va_end(argp);
  snprintf(buffer2,buffer_len,"%s%s",buffer,postfix);
  snprintf(buffer,buffer_len,"%s [%s:%d,%s]",buffer2,relative_file,line,core->getClock()->getXMLDate().c_str());
  if (out!=stdout) {
    if (out) {
      fprintf(out,"%-7s: ",prefix);
      fprintf(out,"%s\n",buffer);
      fflush(out);
    }
  } else {

    // Output android message
    if (!messageLogOnly) {
      __android_log_write(logPrio,"GDCore",buffer);
      GDApp_addMessage(verbosity,"GDCore",buffer2);
    }
    buffer2[0]=toupper(buffer2[0]);
    std::string message=std::string(buffer2);

    // Output message to file additionally
    if (messagelog) {
      fprintf(messagelog,"%-7s: ",prefix);
      fprintf(messagelog,"%s\n",buffer);
      fflush(messagelog);
    }

    // Create dialog if required
    if (!messageLogOnly) {
      switch(verbosity) {
        case verbosityError:
          GDApp_executeAppCommand("errorDialog(\"" + message + "\")");
          break;
        case verbosityWarning:
          GDApp_executeAppCommand("warningDialog(\"" + message + "\")");
          break;
        case verbosityFatal:
          //GDApp_executeAppCommand("fatalDialog(\"" + message + "\")");
          break;
        case verbosityInfo:
          GDApp_executeAppCommand("infoDialog(\"" + message + "\")");
          break;
        case verbosityDebug:
          break;
        case verbosityTrace:
          break;
      }
    }

    /* Wait for ever after a fatal
    if (fatalOccured) {
      while(1) sleep(1);
    }*/

    // Trigger a crash if a fatal error has occured
    if (fatalOccured) {
      GDApp_handleFatal();
    }
  }
}

}
