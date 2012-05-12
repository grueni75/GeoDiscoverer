//============================================================================
// Name        : Thread.cpp
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
Thread::Thread() {
}

// Creates a thread
ThreadInfo *Thread::createThread(ThreadFunction threadFunction, void *threadArgument) {
  ThreadInfo *thread;
  pthread_attr_t attr;
  struct sched_param param;
  if (!(thread=(ThreadInfo *)malloc(sizeof(ThreadInfo)))) {
    FATAL("can not reserve memory for thread structure",NULL);
    return NULL;
  }
  pthread_attr_init(&attr);
  int rc = pthread_create(thread, &attr, threadFunction, (void *)threadArgument);
  if (rc) {
    FATAL("can not create thread object",NULL);
    free(thread);
    return NULL;
  }
  /*pthread_attr_getschedparam(&attr, &param);
  param.sched_priority = priority;
  Int result=pthread_setschedparam(*thread, SCHED_OTHER, &param);
  if (result!=0) {
    FATAL("can not set thread priority (error=%d)",result);
  }*/
  return thread;
}

// Destroys a thread
void Thread::destroyThread(ThreadInfo *thread) {
  //pthread_cancel(*thread);
  free(thread);
}

// Waits until the thread exists
void Thread::waitForThread(ThreadInfo *thread) {
  void *status;
  if (pthread_join(*thread,&status)) {
    FATAL("can not join thread object",NULL);
  }
}

// Detachs a thread from the current running thread
void Thread::detachThread(ThreadInfo *thread) {
  pthread_detach(*thread);
}


// Creates a mutex
ThreadMutexInfo *Thread::createMutex() {
  ThreadMutexInfo *m;
  if (!(m=(ThreadMutexInfo *)malloc(sizeof(ThreadMutexInfo)))) {
    FATAL("can not reserve memory for mutex structure",NULL);
    return NULL;
  }
  pthread_mutex_init(m,NULL);
  return m;
}

// Destroys a mutex
void Thread::destroyMutex(ThreadMutexInfo *mutex) {
  pthread_mutex_destroy(mutex);
  free(mutex);
}

// Locks a mutex
void Thread::lockMutex(ThreadMutexInfo *mutex) {
  pthread_mutex_lock(mutex);
}

// Unlocks a mutex
void Thread::unlockMutex(ThreadMutexInfo *mutex) {
  pthread_mutex_unlock(mutex);
}

// Creates a signal
ThreadSignalInfo *Thread::createSignal(bool oneTimeOnly) {
  ThreadSignalInfo *i;
  if (!(i=(ThreadSignalInfo *)malloc(sizeof(ThreadSignalInfo)))) {
    FATAL("can not reserve memory for signal structure",NULL);
    return NULL;
  }
  pthread_cond_init(&i->cv,NULL);
  pthread_mutex_init(&i->mutex,NULL);
  i->issued=false;
  i->oneTimeOnly=oneTimeOnly;
  return i;
}

// Destroys a signal
void Thread::destroySignal(ThreadSignalInfo *signal) {
  pthread_cond_destroy(&signal->cv);
  pthread_mutex_destroy(&signal->mutex);
  free(signal);
}

// Issues a signal
void Thread::issueSignal(ThreadSignalInfo *signal) {
  pthread_mutex_lock(&signal->mutex);
  signal->issued=true;
  pthread_cond_signal(&signal->cv);
  pthread_mutex_unlock(&signal->mutex);
}

// Waits for a signal
void Thread::waitForSignal(ThreadSignalInfo *signal) {
  pthread_mutex_lock(&signal->mutex);
  while(!signal->issued) {
    pthread_cond_wait(&signal->cv,&signal->mutex);
  }
  if (!signal->oneTimeOnly)
    signal->issued=false;
  pthread_mutex_unlock(&signal->mutex);
}

// Exits a thead (has to be called by thread itself)
void Thread::exitThread() {
  pthread_exit(NULL);
}

// Let other thread run
void Thread::reschedule() {
  sched_yield();
  //usleep(0);
}

// Destructor
Thread::~Thread() {
}

}
