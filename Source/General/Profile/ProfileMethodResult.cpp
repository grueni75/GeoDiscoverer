//============================================================================
// Name        : ProfileMethodResult.cpp
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

namespace GEODISCOVERER {

// Constructor
ProfileMethodResult::ProfileMethodResult() {
}

// Destructor
ProfileMethodResult::~ProfileMethodResult() {
  for(ProfileBlockResultMap::iterator i=blockResultMap.begin();i!=blockResultMap.end();i++) {
    delete i->second;
  }
}

// Outputs the result
void ProfileMethodResult::outputResult(bool clear) {

  double totalAvgDuration=0;
  TimestampInMicroseconds totalMaxDuration=0;
  TimestampInMicroseconds totalMinDuration=0;
  TimestampInMicroseconds totalTotalDuration=0;

  // Sort according to average duration
  std::list<ProfileBlockResult*> sortedList;
  for(ProfileBlockResultMap::iterator i=blockResultMap.begin();i!=blockResultMap.end();i++) {
    double avgDuration=i->second->getAvgDuration();
    totalAvgDuration+=avgDuration;
    totalMaxDuration+=i->second->getMaxDuration();
    totalMinDuration+=i->second->getMinDuration();
    totalTotalDuration+=i->second->getTotalDuration();
    bool inserted=false;
    for(std::list<ProfileBlockResult*>::iterator j=sortedList.begin();j!=sortedList.end();j++) {
      ProfileBlockResult *d=*j;
      if (avgDuration>=d->getAvgDuration()) {
        inserted=true;
        sortedList.insert(j,i->second);
        break;
      }
    }
    if (!inserted)
      sortedList.push_back(i->second);
  }

  // Output result
  for(std::list<ProfileBlockResult*>::iterator j=sortedList.begin();j!=sortedList.end();j++) {
    (*j)->outputResult(totalMinDuration,totalAvgDuration,totalMaxDuration,totalTotalDuration,clear);
  }
}

// Returns the result of the block with the given name
ProfileBlockResult *ProfileMethodResult::getBlockResult(std::string name) {

  // Find or add the block
  ProfileBlockResult *result;
  ProfileBlockResultMap::iterator i;
  i=blockResultMap.find(name);
  if (i!=blockResultMap.end()) {
    result=i->second;
  } else {
    if (!(result=new ProfileBlockResult())) {
      FATAL("can not create profile block result object",NULL);
      return NULL;
    }
    result->setName(name);
    blockResultMap[name]=result;
  }
  return result;

}

}
