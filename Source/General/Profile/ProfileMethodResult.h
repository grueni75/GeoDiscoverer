//============================================================================
// Name        : ProfileMethodResult.h
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


#ifndef PROFILEMETHODRESULT_H_
#define PROFILEMETHODRESULT_H_

namespace GEODISCOVERER {

// Typedefs for thr result hash
typedef std::map<std::string, ProfileBlockResult*> ProfileBlockResultMap;
typedef std::pair<std::string, ProfileBlockResult*> ProfileBlockResultPair;

class ProfileMethodResult {

protected:

  // Name of the method
  std::string name;

  // Time of last interaction
  TimestampInMicroseconds lastTimestamp;

  // Map of block result
  ProfileBlockResultMap blockResultMap;

public:

  // Constructor
  ProfileMethodResult();

  // Destructor
  virtual ~ProfileMethodResult();

  // Returns the result of the block with the given name
  ProfileBlockResult *getBlockResult(std::string name);

  // Outputs the result
  void outputResult(bool clear);

  // Getters and setters
  TimestampInMicroseconds getLastTimestamp() const
  {
      return lastTimestamp;
  }

  void setLastTimestamp(TimestampInMicroseconds lastTimestamp)
  {
      this->lastTimestamp = lastTimestamp;
  }

  void setName(std::string name)
  {
      this->name = name;
  }

  std::string getName() const
  {
      return name;
  }
};

}

#endif /* PROFILEMETHODRESULT_H_ */
