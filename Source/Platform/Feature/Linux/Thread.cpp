//============================================================================
// Name        : Thread.cpp
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

namespace GEODISCOVERER {

// Sets the priority of a thread
void Thread::setThreadPriority(ThreadPriority priority) {
  switch(priority) {
    case threadPriorityForegroundHigh:
      nice(0);
      break;
    case threadPriorityForegroundLow:
      nice(1);
      break;
    case threadPriorityBackgroundHigh:
      nice(2);
      break;
    case threadPriorityBackgroundLow:
      nice(3);
      break;
    default:
      FATAL("unsupported thread priority",NULL);
  }
}

}




