//============================================================================
// Name        : ProfileEngine.cpp
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

// Constructor
ProfileEngine::ProfileEngine() {
  accessMutex=core->getThread()->createMutex("profile engine access mutex");
}

// Destructor
ProfileEngine::~ProfileEngine() {
  for(ProfileMethodResultMap::iterator i=methodResultMap.begin();i!=methodResultMap.end();i++) {
    delete i->second;
  }
  core->getThread()->destroyMutex(accessMutex);
}

// Starts a new measurement series
void ProfileEngine::startMeasure(std::string method) {

  // Ensure that only one instance is executing this code
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Get the data entry from the map
  ProfileMethodResult *result;
  ProfileMethodResultMap::iterator i;
  i=methodResultMap.find(method);
  if (i!=methodResultMap.end()) {
    result=i->second;
  } else {
    if (!(result=new ProfileMethodResult())) {
      FATAL("can not create profile method result object",NULL);
      return;
    }
    result->setName(method);
    methodResultMap[method]=result;
  }

  // Update the timestamp
  result->setLastTimestamp(core->getClock()->getMicrosecondsSinceStart());

  // Release mutex
  core->getThread()->unlockMutex(accessMutex);
}

// Remember the currently elapsed time under the given name
void ProfileEngine::addElapsedTime(std::string method, std::string name) {

  // Ensure that only one instance is executing this code
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Get the elapsed time
  TimestampInMicroseconds currentTimestamp=core->getClock()->getMicrosecondsSinceStart();
  ProfileMethodResultMap::iterator i;
  ProfileMethodResult *methodResult;
  i=methodResultMap.find(method);
  if (i!=methodResultMap.end()) {
    methodResult=i->second;
  } else {
    DEBUG("measurement for method <%s> has not been started",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  }
  TimestampInMicroseconds timeDiff=currentTimestamp-methodResult->getLastTimestamp();

  // Check if the elasped time makes sense
  if (timeDiff/(1000*1000)>60*60) {
    DEBUG("time difference does not make sense (currentTimestamp=%ld, lastTimestamp=%ld)",currentTimestamp,methodResult->getLastTimestamp());
  } else {

    // Get the data entry from the map
    ProfileBlockResult *blockResult=methodResult->getBlockResult(name);

    // Update it
    blockResult->updateDuration(timeDiff);

  }

  // Release mutex
  core->getThread()->unlockMutex(accessMutex);

  // Start the measurement
  startMeasure(method);
}

// Outputs the collected time measurements
void ProfileEngine::outputResult(std::string method, bool clear) {

  // Ensure that only one instance is executing this code
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Sort according to method name
  std::list<ProfileMethodResult*> sortedList;
  for(ProfileMethodResultMap::iterator i=methodResultMap.begin();i!=methodResultMap.end();i++) {
    bool inserted=false;
    std::string name=i->second->getName();
    if ((method=="")||(method==name)) {
      for(std::list<ProfileMethodResult*>::iterator j=sortedList.begin();j!=sortedList.end();j++) {
        ProfileMethodResult *d=*j;
        if (name>=d->getName()) {
          inserted=true;
          sortedList.insert(j,i->second);
          break;
        }
      }
      if (!inserted)
        sortedList.push_back(i->second);
    }
  }

  // Output the list
  for(std::list<ProfileMethodResult*>::iterator j=sortedList.begin();j!=sortedList.end();j++) {
    DEBUG("%s",(*j)->getName().c_str());
    (*j)->outputResult(clear);
  }

  // Release mutex
  core->getThread()->unlockMutex(accessMutex);
}

}
