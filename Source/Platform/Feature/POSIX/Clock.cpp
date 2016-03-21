//============================================================================
// Name        : Time.cpp
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

// Returns a date in the given format
std::string Clock::getFormattedDate(TimestampInSeconds timestamp, std::string format, bool asLocalTime) {
  const int buffer_len=256;
  char date[buffer_len];
  time_t t;
  struct tm *tmp;
  if (timestamp!=0)
    t=timestamp;
  else
    t=time(NULL);
  if (asLocalTime)
    tmp=localtime(&t);
  else {
    tmp=gmtime(&t);
  }
  strftime(date,buffer_len,format.c_str(),tmp);
  std::string result=date;
  return result;
}

// Returns a formatted date string
std::string Clock::getFormattedDate() {
  return getFormattedDate(getSecondsSinceEpoch(),"%Y%m%d-%H%M%S",true);
}

// Returns a formatted date string suitable for XML files
std::string Clock::getXMLDate(TimestampInSeconds timestamp, bool asLocalTime) {
  std::string result=getFormattedDate(timestamp,"%Y-%m-%dT%H:%M:%S",asLocalTime);
  if (!asLocalTime)
    result=result+"Z";
  return result;
}

// Returns a timestamp from a given XML date string
TimestampInSeconds Clock::getXMLDate(std::string timestamp, bool asLocalTime) {
  struct tm tm;
  struct tm *tm2;
  char *tz;
  time_t ret,ret2;
  memset(&tm,0,sizeof(struct tm));
  strptime(timestamp.c_str(),"%Y-%m-%dT%H:%M:%S",&tm);
  if (asLocalTime) {
    return mktime(&tm);  // result is in local time;
  } else {

    // Manual computation required
    // Android does not set tm.tm_yday

    // Is the year a leap year?
    Int year = tm.tm_year + 1900;
    bool leapYear;
    if (year%4!=0)
      leapYear=false;
    else if (year%100!=0)
      leapYear=true;
    else if (year%400!=0) 
      leapYear=false;
    else
      leapYear=true;

    // Convert month into year days
    Int mday[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    tm.tm_yday=0;
    for (Int i=0;i<12;i++) {
      if (i==tm.tm_mon)
        break;
      tm.tm_yday+=mday[i];
      if ((i==1)&&(leapYear))
        tm.tm_yday++;
    }

    // Add the month days
    tm.tm_yday+=tm.tm_mday-1;

    // Convert back to time since epoch
    ret = (time_t)tm.tm_sec + (time_t)tm.tm_min*60 + (time_t)tm.tm_hour*3600 + (time_t)tm.tm_yday*86400 +
          (time_t)(tm.tm_year-70)*31536000 + (time_t)((tm.tm_year-69)/4)*86400 -
          (time_t)((tm.tm_year-1)/100)*86400 + (time_t)((tm.tm_year+299)/400)*86400;
    return ret;
  }

  return (TimestampInSeconds)ret;
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
