//============================================================================
// Name        : Thread.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

namespace GEODISCOVERER {

typedef void *(*ThreadFunction)(void *);
typedef pthread_t ThreadInfo;
typedef struct ThreadMutexInfo {
  pthread_mutex_t pthreadMutex;
  ThreadInfo lockedThread;
  Int lockedCount;
} ThreadMutexInfo;
typedef struct ThreadSignalInfo {
  pthread_mutex_t mutex;
  pthread_cond_t cv;
  bool issued;
  bool oneTimeOnly;
} ThreadSignalInfo;
enum ThreadPriority { threadPriorityForeground=0, threadPriorityBackgroundHigh=1, threadPriorityBackgroundLow=2 };

typedef std::map<ThreadInfo, std::string*> ThreadNameMap;
typedef std::pair<ThreadInfo, std::string*> ThreadNamePair;
typedef std::map<ThreadMutexInfo*, std::string*> ThreadMutexNameMap;
typedef std::pair<ThreadMutexInfo*, std::string*> ThreadMutexNamePair;
typedef std::map<ThreadMutexInfo*, std::list<std::string*> *> ThreadMutexWaitQueueMap;
typedef std::pair<ThreadMutexInfo*, std::list<std::string*> *> ThreadMutexWaitQueuePair;

class Thread {

protected:

  // Maps thread ids to thread names
  ThreadNameMap threadNameMap;

  // Maps mutex pointers to mutex names
  ThreadMutexNameMap mutexNameMap;

  // Contains all threads that are currently waiting for a mutex
  ThreadMutexWaitQueueMap mutexWaitQueueMap;

  // Thread that outputs all threads waiting for a mute
  ThreadInfo *mutexDebugThreadInfo;

  // Mutex to control access to the maps
  pthread_mutex_t accessMutex;

  // Decides if mutex locking infos shall be logged
  bool createMutexLog;

public:
  Thread();

  // Creates a thread
  ThreadInfo *createThread(std::string name, ThreadFunction threadFunction, void *threadArgument);

  // Sets the priority of a thread
  void setThreadPriority(ThreadPriority priority);

  // Destroys a thread
  void destroyThread(ThreadInfo *thread);

  // Detachs a thread from the current running thread
  void detachThread(ThreadInfo *thread);

  // Waits until the thread exists
  void waitForThread(ThreadInfo *thread);

  // Creates a mutex
  ThreadMutexInfo *createMutex(std::string name);

  // Destroys a mutex
  void destroyMutex(ThreadMutexInfo *mutex);

  // Locks a mutex
  void lockMutex(ThreadMutexInfo *mutex, const char *file, int line, bool debugMsgs=false);

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
  bool waitForSignal(ThreadSignalInfo *signal, TimestampInMilliseconds maxWaitTime=0);

  // Let other thread run
  void reschedule();

  // Exits a thead (has to be called by thread itself)
  void exitThread();

  // Allow cancellation of the thread
  void setThreadCancable();

  // Cancel the thread
  bool cancelThread(ThreadInfo *thread);

  // Thread function that debugs mute locks
  void debugMutexLocks();

  // Destructor
  virtual ~Thread();
};

}

#endif /* THREAD_H_ */
