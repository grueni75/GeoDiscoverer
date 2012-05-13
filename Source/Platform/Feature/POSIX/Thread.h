//============================================================================
// Name        : Thread.h
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

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

namespace GEODISCOVERER {

typedef void *(*ThreadFunction)(void *);
typedef pthread_t ThreadInfo;
typedef pthread_mutex_t ThreadMutexInfo;
typedef struct ThreadSignalInfo {
  pthread_mutex_t mutex;
  pthread_cond_t cv;
  bool issued;
  bool oneTimeOnly;
} ThreadSignalInfo;
enum ThreadPriority { threadPriorityForeground=0, threadPriorityBackgroundHigh=1, threadPriorityBackgroundLow=2 };

class Thread {

public:
  Thread();

  // Creates a thread
  ThreadInfo *createThread(ThreadFunction threadFunction, void *threadArgument);

  // Sets the priority of a thread
  void setThreadPriority(ThreadPriority priority);

  // Destroys a thread
  void destroyThread(ThreadInfo *thread);

  // Detachs a thread from the current running thread
  void detachThread(ThreadInfo *thread);

  // Waits until the thread exists
  void waitForThread(ThreadInfo *thread);

  // Creates a mutex
  ThreadMutexInfo *createMutex();

  // Destroys a mutex
  void destroyMutex(ThreadMutexInfo *mutex);

  // Locks a mutex
  void lockMutex(ThreadMutexInfo *mutex);

  // Unlocks a mutex
  void unlockMutex(ThreadMutexInfo *mutex);

  // Creates a signal (if oneTimeOnly is set to true, signal can only be issued one time)
  // Please note that if oneTimeOnly is false, the created signal can only be consumed by one thread only
  ThreadSignalInfo *createSignal(bool oneTimeOnly=false);

  // Destroys a signal
  void destroySignal(ThreadSignalInfo *signal);

  // Issues a signal
  void issueSignal(ThreadSignalInfo *signal);

  // Waits for a signal
  void waitForSignal(ThreadSignalInfo *signal);

  // Let other thread run
  void reschedule();

  // Exits a thead (has to be called by thread itself)
  void exitThread();

  // Destructor
  virtual ~Thread();
};

}

#endif /* THREAD_H_ */
