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

// Quits the thread immediately
void threadExitHandler(int sig)
{
    //DEBUG("received signal %d", sig);
    if (sig==SIGABRT)
      pthread_exit(0);
}

// Mutex debug thread
void *mutexDebugThread(void *args) {
  ((Thread*)args)->debugMutexLocks();
  return NULL;
}

// Constructor
Thread::Thread() {

  // Remember the thread that created this object
  threadNameMap.insert(ThreadNamePair(pthread_self(),new std::string("Core")));

  // Create the thread that outputs all threads waiting for a mutex
  pthread_mutex_init(&accessMutex,NULL);
  createMutexLog=false;
  mutexDebugThreadInfo=createThread("mutex debug thread",mutexDebugThread,this);

}

// Creates a thread
ThreadInfo *Thread::createThread(std::string name, ThreadFunction threadFunction, void *threadArgument) {
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
  pthread_mutex_lock(&accessMutex);
  std::string *t = new std::string(name);
  if (!t) {
    FATAL("can not create string",NULL);
  }
  threadNameMap.insert(ThreadNamePair(*thread,t));
  pthread_mutex_unlock(&accessMutex);
  return thread;
}

// Destroys a thread
void Thread::destroyThread(ThreadInfo *thread) {
  //pthread_cancel(*thread);
  pthread_mutex_lock(&accessMutex);
  ThreadNameMap::iterator i = threadNameMap.find(*thread);
  if (i!=threadNameMap.end()) {
    delete i->second;
    threadNameMap.erase(i);
  }
  pthread_mutex_unlock(&accessMutex);
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
ThreadMutexInfo *Thread::createMutex(std::string name) {
  ThreadMutexInfo *m;
  if (!(m=(ThreadMutexInfo *)malloc(sizeof(ThreadMutexInfo)))) {
    FATAL("can not reserve memory for mutex structure",NULL);
    return NULL;
  }
  pthread_mutex_init(&m->pthreadMutex,NULL);
  m->lockedThread=0;
  m->lockedCount=0;
  pthread_mutex_lock(&accessMutex);
  std::stringstream s;
  s << name << " (0x" << std::stringstream::hex << m << ")";
  std::string *t = new std::string(s.str());
  if (!t) {
    FATAL("can not create string",NULL);
  }
  mutexNameMap.insert(ThreadMutexNamePair(m,t));
  pthread_mutex_unlock(&accessMutex);
  return m;
}

// Destroys a mutex
void Thread::destroyMutex(ThreadMutexInfo *mutex) {
  pthread_mutex_lock(&accessMutex);
  ThreadMutexWaitQueueMap::iterator j = mutexWaitQueueMap.find(mutex);
  if (j!=mutexWaitQueueMap.end()) {
    for(std::list<std::string*>::iterator k=j->second->begin();k!=j->second->end();k++) {
      delete *k;
    }
    delete j->second;
    mutexWaitQueueMap.erase(j);
  }
  ThreadMutexNameMap::iterator i = mutexNameMap.find(mutex);
  if (i!=mutexNameMap.end()) {
    delete i->second;
    mutexNameMap.erase(i);
  }
  pthread_mutex_unlock(&accessMutex);
  pthread_mutex_destroy(&mutex->pthreadMutex);
  free(mutex);
}

// Locks a mutex
void Thread::lockMutex(ThreadMutexInfo *mutex, const char *file, int line, bool debugMsgs) {
  pthread_t self = pthread_self();
  std::list<std::string*> *waitQueue=NULL;
  std::string *threadNameCopy=NULL;
  if (mutex->lockedThread==self) {
    mutex->lockedCount++;
    return; // already locked
  }
  if (createMutexLog) {
    pthread_mutex_lock(&accessMutex);
    if (mutexWaitQueueMap.find(mutex)!=mutexWaitQueueMap.end()) {
      waitQueue=mutexWaitQueueMap[mutex];
    } else {
      if (!(waitQueue=new std::list<std::string*>())) {
        FATAL("can not create mutex wait queue",NULL);
        return;
      }
      mutexWaitQueueMap.insert(ThreadMutexWaitQueuePair(mutex,waitQueue));
    }
    std::string *threadName = threadNameMap[self];
    if (threadName==NULL) {
      std::stringstream s;
      s << "unnamed thread 0x" << std::hex << std::setw(8) << self;
      threadName = new std::string(s.str());
      if (!threadName) {
        FATAL("can not create string",NULL);
        return;
      }
      threadNameMap[self]=threadName;
    }
    const char *relativeFile=strstr(file,SRC_ROOT);
    if (!relativeFile)
      relativeFile=file;
    else
      relativeFile=file+strlen(SRC_ROOT)+1;
    std::stringstream s;
    s << *threadName << " [" << relativeFile << ":" << line << "]";
    threadNameCopy = new std::string(s.str());
    if (!threadNameCopy) {
      FATAL("can not create thread name string",NULL);
      return;
    }
    waitQueue->push_back(threadNameCopy);
    if (debugMsgs)
      DEBUG("wait for lock: %s",threadNameCopy->c_str());
    pthread_mutex_unlock(&accessMutex);
  }
  pthread_mutex_lock(&mutex->pthreadMutex);
  mutex->lockedThread=self;
  mutex->lockedCount=1;
  if ((createMutexLog)&&(threadNameCopy)) {
    //DEBUG("mutex %s locked by %s",mutexNameMap[mutex]->c_str(),threadNameCopy->c_str());
    pthread_mutex_lock(&accessMutex);
    waitQueue->remove(threadNameCopy);
    std::string *threadNameLocked = new std::string(*threadNameCopy + " (locked)");
    if (threadNameLocked==NULL) {
      FATAL("can not create locked thread name string",NULL);
      return;
    }
    delete threadNameCopy;
    for(std::list<std::string*>::iterator i=waitQueue->begin();i!=waitQueue->end();i++) {
      std::string *entry = *i;
      if (entry->find("locked")!=std::string::npos) {
        FATAL("mutex locked twice!",NULL);
        return;
      }
    }
    waitQueue->push_back(threadNameLocked);
    if (debugMsgs)
      DEBUG("got lock: %s",threadNameLocked->c_str());
    pthread_mutex_unlock(&accessMutex);
  }
}

// Unlocks a mutex
void Thread::unlockMutex(ThreadMutexInfo *mutex) {
  mutex->lockedCount--;
  if (mutex->lockedCount!=0)
    return;
  mutex->lockedThread=0;
  if (createMutexLog) {
    pthread_mutex_lock(&accessMutex);
    std::list<std::string*> *waitQueue;
    if (mutexWaitQueueMap.find(mutex)!=mutexWaitQueueMap.end()) {
      pthread_t self = pthread_self();
      waitQueue=mutexWaitQueueMap[mutex];
      std::string *threadName = threadNameMap[self];
      if (threadName) {
        std::string prefix = *threadName + " [";
        for(std::list<std::string*>::iterator i=waitQueue->begin();i!=waitQueue->end();i++) {
          std::string *entry = *i;
          if (entry->substr(0, prefix.size())==prefix) {
            delete entry;
            waitQueue->erase(i);
            break;
          }
        }
        //DEBUG("mutex %s unlocked by %s",mutexNameMap[mutex]->c_str(),threadName->c_str());
      }
    }
    pthread_mutex_unlock(&accessMutex);
  }
  pthread_mutex_unlock(&mutex->pthreadMutex);
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
bool Thread::waitForSignal(ThreadSignalInfo *signal, TimestampInMilliseconds maxWaitTime) {
  pthread_mutex_lock(&signal->mutex);
  if (maxWaitTime>0) {
    if(!signal->issued) {
      struct timeval tv;
      struct timespec ts;
      gettimeofday(&tv, NULL);
      ts.tv_sec = time(NULL) + maxWaitTime / 1000;
      ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (maxWaitTime % 1000);
      ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
      ts.tv_nsec %= (1000 * 1000 * 1000);
      pthread_cond_timedwait(&signal->cv, &signal->mutex, &ts);
    }
  } else {
    while(!signal->issued) {
      pthread_cond_wait(&signal->cv,&signal->mutex);
    }
  }
  bool result=signal->issued;
  if (!signal->oneTimeOnly)
    signal->issued=false;
  pthread_mutex_unlock(&signal->mutex);
  return result;
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

// Allow cancellation of the thread
void Thread::setThreadCancable() {
  //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  struct sigaction actions;
  memset(&actions, 0, sizeof(actions));
  sigemptyset(&actions.sa_mask);
  actions.sa_flags = 0;
  actions.sa_handler = threadExitHandler;
  if (sigaction(SIGABRT,&actions,NULL)!=0)
    FATAL("can not set exit handler for thread",NULL);
}

// Cancel the thread
bool Thread::cancelThread(ThreadInfo *thread) {
  //pthread_cancel(*thread);
  //DEBUG("signaling thread %d",*thread);
  Int rc = pthread_kill(*thread, SIGABRT);
  if ((rc != 0)&&(rc != 3)) {
    FATAL("can not kill thread",NULL);
  }
  if (rc == 3)
    return false;
  else
    return true;
}

// Destructor
Thread::~Thread() {
  pthread_mutex_lock(&accessMutex);
  if (cancelThread(mutexDebugThreadInfo)) {
    waitForThread(mutexDebugThreadInfo);
  }
  free(mutexDebugThreadInfo);
  pthread_mutex_unlock(&accessMutex);
  pthread_mutex_destroy(&accessMutex);
  for(ThreadMutexWaitQueueMap::iterator i=mutexWaitQueueMap.begin();i!=mutexWaitQueueMap.end();i++) {
    for(std::list<std::string*>::iterator j=i->second->begin();j!=i->second->end();j++) {
      delete *j;
    }
    delete i->second;
  }
  mutexWaitQueueMap.clear();
  for(ThreadMutexNameMap::iterator i=mutexNameMap.begin();i!=mutexNameMap.end();i++) {
    delete i->second;
  }
  mutexNameMap.clear();
  for(ThreadNameMap::iterator i=threadNameMap.begin();i!=threadNameMap.end();i++) {
    delete i->second;
  }
  threadNameMap.clear();
}

// Thread function that debugs mute locks
void Thread::debugMutexLocks() {

  // Wait until the core object is available
  while (core==NULL) {
    sleep(1);
  }
  while (core->getConfigStore()==NULL) {
    sleep(1);
  }

  // Check if we shall create a mutex debug log
  pthread_mutex_lock(&accessMutex);
  createMutexLog = core->getConfigStore()->getIntValue("General","createMutexLog", __FILE__, __LINE__);
  pthread_mutex_unlock(&accessMutex);
  if (!createMutexLog) {
    return;
  }

  // Prepare variables
  Int waitTime = core->getConfigStore()->getIntValue("General","mutexLogUpdateInterval", __FILE__, __LINE__);
  FILE *mutexDebugLog;
  std::string logPath=core->getHomePath() + "/Log/mutex-" + core->getClock()->getFormattedDate() + ".log";

  // Set the priority
  setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  setThreadCancable();

  // Do an endless loop
  while (1) {

    // Wait a little bit
    sleep(waitTime);

    // Open the mutex debug log
    pthread_mutex_lock(&accessMutex);
    Int tries;
    for(tries=0;tries<core->getFileOpenForWritingRetries();tries++) {
      DEBUG("tries=%d",tries);
      if ((mutexDebugLog=fopen(logPath.c_str(),"w"))) {
        break;
      }
      usleep(core->getFileOpenForWritingWaitTime());
    }
    if (tries>=core->getFileOpenForWritingRetries()) {
      FATAL("can not open mutex log for writing!",NULL);
      return;
    } else {
      for(ThreadMutexWaitQueueMap::iterator i=mutexWaitQueueMap.begin();i!=mutexWaitQueueMap.end();i++) {
        std::list<std::string*> *waitQueue = i->second;
        if (!waitQueue->empty()) {
          std::string *mutexName = mutexNameMap[i->first];
          std::stringstream buffer;
          buffer << "-----------------------------------------------------------------------------" << "\n";
          buffer << *mutexName << "\n";
          buffer << "-----------------------------------------------------------------------------" << "\n";
          for(std::list<std::string*>::iterator i=waitQueue->begin();i!=waitQueue->end();i++) {
            buffer << *(*i) << "\n";
          }
          buffer << "\n";
          fwrite(buffer.str().c_str(),buffer.str().length(),1,mutexDebugLog);
        }
      }
      fclose(mutexDebugLog);
    }
    pthread_mutex_unlock(&accessMutex);

  }
}

}
