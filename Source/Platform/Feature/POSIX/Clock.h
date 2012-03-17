//============================================================================
// Name        : Time.h
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

  // Returns a formatted date string suitable for XML files
  std::string getXMLDate(TimestampInSeconds timestamp=0);

  // Returns a timestamp from a given XML date string
  TimestampInSeconds getXMLDate(std::string timestamp);

  // Returns the current time in microseconds since epoch
  TimestampInMicroseconds getMicrosecondsSinceStart();

};

}

#endif /* CLOCK_H_ */
