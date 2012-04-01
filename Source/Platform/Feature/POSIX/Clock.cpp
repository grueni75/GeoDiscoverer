//============================================================================
// Name        : Time.cpp
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
Clock::Clock() {

  // Remember the start time
  startTime=getSecondsSinceEpoch();

  // Do some sanity checks
  if (sizeof(TimestampInMicroseconds)!=8) {
    puts("FATAL: microseconds timestamp is not a 64-bit integer!");
    exit(1);
  }
}

// Destructor
Clock::~Clock() {
}

// Returns a formatted date string
std::string Clock::getFormattedDate() {
  const int buffer_len=256;
  char date[buffer_len];
  time_t t;
  t=time(NULL);
  strftime(date,buffer_len,"%Y%m%d-%H%M%S",localtime(&t));
  std::string result=date;
  return result;
}

// Returns a formatted date string suitable for XML files
std::string Clock::getXMLDate(TimestampInSeconds timestamp) {
  const int buffer_len=256;
  char date[buffer_len];
  time_t t;
  if (timestamp!=0)
    t=timestamp;
  else
    t=time(NULL);
  strftime(date,buffer_len,"%Y-%m-%dT%H:%M:%S",localtime(&t));
  std::string result=date;
  return result;
}

// Returns a timestamp from a given XML date string
TimestampInSeconds Clock::getXMLDate(std::string timestamp) {
  struct tm tm;
  memset(&tm,0,sizeof(struct tm));
  strptime(timestamp.c_str(),"%Y-%m-%dT%H:%M:%S",&tm);
  return (TimestampInSeconds)mktime(&tm);
}

// Returns the current time in microseconds since epoch
TimestampInMicroseconds Clock::getMicrosecondsSinceStart() {
  struct timeval tv;

  // Get the current time
  if (gettimeofday(&tv,NULL)!=0) {
    FATAL("can not obtain time",NULL);
    return 0;
  }

  // Convert it to a 64-bit timestamp
  TimestampInMicroseconds t=tv.tv_sec-startTime;
  t=t*1000*1000+tv.tv_usec;
  return t;
}


}
