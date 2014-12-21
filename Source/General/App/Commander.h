//============================================================================
// Name        : Commander.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef COMMANDER_H_
#define COMMANDER_H_

namespace GEODISCOVERER {

class Commander {

protected:

  // Mutex to ensure that only one thread is executing a command at a time
  ThreadMutexInfo *accessMutex;

  // Coordinates of the last touched point
  Int lastTouchedX;
  Int lastTouchedY;

public:

  // Constructor and destructor
  Commander();
  virtual ~Commander();

  // Interrupts the commander
  void interruptOperation(const char *file, int line) const {
    core->getThread()->lockMutex(accessMutex, file, line);
  }

  // Continues the commander
  void continueOperation() const {
    core->getThread()->unlockMutex(accessMutex);
  }

  // Execute a command
  std::string execute(std::string cmd);

  // Dispatch a command to the parent app
  void dispatch(std::string cmd);

};

}

#endif /* COMMANDER_H_ */
