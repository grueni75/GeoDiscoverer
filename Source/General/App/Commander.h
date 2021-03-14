//============================================================================
// Name        : Commander.h
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

  // Current number for the map tile
  Int mapTileNr;

public:

  // Constructor and destructor
  Commander();
  virtual ~Commander();

  // Extracts the command name and its arguments from a command
  bool splitCommand(std::string cmdString, std::string& cmd, std::vector<std::string>& args);

  // Creates a command string from the given arguments
  std::string joinCommand(std::string cmd, std::vector<std::string> args);

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
  static std::string dispatch(std::string cmd);

};

}

#endif /* COMMANDER_H_ */
