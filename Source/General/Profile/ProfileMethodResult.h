//============================================================================
// Name        : ProfileMethodResult.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
