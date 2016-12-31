//============================================================================
// Name        : ProfileBlockResult.h
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


#ifndef PROFILEBLOCKRESULT_H_
#define PROFILEBLOCKRESULT_H_

namespace GEODISCOVERER {

class ProfileBlockResult {

protected:

  std::string name;                       // Name of the result
  double totalDuration;                   // Sum of all collected duration measurements
  Int totalCount;                         // Number of measurements
  TimestampInMicroseconds minDuration;    // Minimum duration among all measurements
  TimestampInMicroseconds maxDuration;    // Maximum duration among all measurements

public:

  // Constructor
  ProfileBlockResult();

  // Destructor
  virtual ~ProfileBlockResult();

  // Updates the entry with the given duration
  void updateDuration(TimestampInMicroseconds duration);

  // Outputs the result
  void outputResult(TimestampInMicroseconds totalMinDuration, TimestampInMicroseconds totalAvgDuration, TimestampInMicroseconds totalMaxDuration, TimestampInMicroseconds totalTotalDuration, bool clear);

  // Getters and setters
  void setName(std::string name)
  {
      this->name = name;
  }

  TimestampInMicroseconds getAvgDuration();

  TimestampInMicroseconds getMaxDuration() const
  {
      return maxDuration;
  }

  TimestampInMicroseconds getMinDuration() const
  {
      return minDuration;
  }

  double getTotalDuration() const {
    return totalDuration;
  }
};

}

#endif /* PROFILEBLOCKRESULT_H_ */
