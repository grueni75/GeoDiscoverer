//============================================================================
// Name        : Thread.cpp
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

// Sets the priority of a thread
void Thread::setThreadPriority(ThreadPriority priority) {
  switch(priority) {
    case threadPriorityForeground:
      nice(0);
      break;
    case threadPriorityBackgroundHigh:
      nice(1);
      break;
    case threadPriorityBackgroundLow:
      nice(2);
      break;
    default:
      FATAL("unsupported thread priority",NULL);
  }
}

}




