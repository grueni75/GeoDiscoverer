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

// Sets the thread priority on the java side
void GDApp_setThreadPriority(int priority);

namespace GEODISCOVERER {

// Sets the priority of a thread
void Thread::setThreadPriority(ThreadPriority priority) {
  GDApp_setThreadPriority(priority);
}

}




