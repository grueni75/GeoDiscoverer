//============================================================================
// Name        : Time.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef CLOCK_H_
#define ClOCK_H_

namespace GEODISCOVERER {

typedef time_t TimestampInSeconds;
typedef unsigned long long TimestampInMicroseconds;
typedef unsigned long long TimestampInMilliseconds;

class Clock {

protected:

  TimestampInSeconds startTime;   // Start time of the program

public:
  Clock();
  virtual ~Clock();

  // Returns the current time in seconds since epoch
  TimestampInSeconds getSecondsSinceEpoch() {
    return (TimestampInSeconds)time(NULL);
  }

  // Returns a formatted time string
  std::string getFormattedDate();

  // Returns a date in the given format
  std::string getFormattedDate(TimestampInSeconds timestamp, std::string format, bool asLocalTime);

  // Returns a formatted date string suitable for XML files
  std::string getXMLDate(TimestampInSeconds timestamp=0, bool asLocalTime=true);

  // Returns a timestamp from a given XML date string
  TimestampInSeconds getXMLDate(std::string timestamp, bool asLocalTime=true);

  // Returns the current time in microseconds since epoch
  TimestampInMicroseconds getMicrosecondsSinceStart();

};

}

#endif /* CLOCK_H_ */
