//============================================================================
// Name        : ProfileBlockResult.cpp
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
ProfileBlockResult::ProfileBlockResult() {
  name="unknown";
  totalDuration=0;
  totalCount=0;
  minDuration=std::numeric_limits<TimestampInMicroseconds>::max();
  maxDuration=0;
}

// Destructor
ProfileBlockResult::~ProfileBlockResult() {
}

// Updates the entry with the given duration
void ProfileBlockResult::updateDuration(TimestampInMicroseconds duration) {
  totalDuration+=duration;
  totalCount++;
  if (duration<minDuration)
    minDuration=duration;
  if (duration>maxDuration)
    maxDuration=duration;
}

// Outputs the result
void ProfileBlockResult::outputResult(TimestampInMicroseconds totalMinDuration, TimestampInMicroseconds totalAvgDuration, TimestampInMicroseconds totalMaxDuration, TimestampInMicroseconds totalTotalDuration, bool clear) {
  DEBUG("%-40s: min=%8.2fms (%3.0f%%) * avg=%8.2fms (%3.0f%%) * max=%8.2fms (%3.0f%%) * total=%10.2fms (%3.0f%%)",name.c_str(),
        (double)minDuration/1000.0,(double)minDuration/(double)totalMinDuration*100,
        (double)getAvgDuration()/1000.0,(double)getAvgDuration()/(double)totalAvgDuration*100,
        (double)maxDuration/1000.0,(double)maxDuration/(double)totalMaxDuration*100,
        (double)totalDuration/1000.0,(double)totalDuration/(double)totalTotalDuration*100);
  if (clear) {
    totalDuration=0;
    totalCount=0;
    minDuration=std::numeric_limits<TimestampInMicroseconds>::max();
    maxDuration=0;
  }
}

// Returns the average duration
TimestampInMicroseconds ProfileBlockResult::getAvgDuration() {
  return round((double)totalDuration/(double)totalCount);
}

}
