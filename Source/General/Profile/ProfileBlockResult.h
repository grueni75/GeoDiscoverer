//============================================================================
// Name        : ProfileBlockResult.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
