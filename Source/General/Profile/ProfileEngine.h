//============================================================================
// Name        : ProfileEngine.h
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


#ifndef PROFILEENGINE_H_
#define PROFILEENGINE_H_

namespace GEODISCOVERER {

// Typedefs for the result hash
typedef std::map<std::string, ProfileMethodResult*> ProfileMethodResultMap;
typedef std::pair<std::string, ProfileMethodResult*> ProfileMethodResultPair;

// Macros
//#define PROFILING_ENABLED
#ifdef PROFILING_ENABLED
#define PROFILE_START core->getProfileEngine()->startMeasure(__PRETTY_FUNCTION__);
#define PROFILE_ADD(name) core->getProfileEngine()->addElapsedTime(__PRETTY_FUNCTION__,name);
#define PROFILE_END core->getProfileEngine()->outputResult(__PRETTY_FUNCTION__,false);
#else
#define PROFILE_START ;
#define PROFILE_ADD(name) ;
#define PROFILE_END ;
#endif

class ProfileEngine {

protected:

  // Hash of collected results
  ProfileMethodResultMap methodResultMap;

  // Mutex for accessing the engine
  ThreadMutexInfo *accessMutex;

public:

  // Constructor
  ProfileEngine();

  // Destructor
  virtual ~ProfileEngine();

  // Starts a new measurement series
  void startMeasure(std::string method);

  // Remember the currently elapsed time under the given name
  void addElapsedTime(std::string method, std::string name);

  // Outputs the collected time measurements
  void outputResult(std::string method, bool clear);

};

}

#endif /* PROFILEENGINE_H_ */
