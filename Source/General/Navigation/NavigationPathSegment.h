//============================================================================
// Name        : NavigationPathSegment.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef NAVIGATIONPATHSEGMENT_H_
#define NAVIGATIONPATHSEGMENT_H_

namespace GEODISCOVERER {

class NavigationPathSegment {

protected:

  NavigationPath *path;                       // The path that contains this segment
  NavigationPathVisualization *visualization; // The visualization of the path
  Int startIndex;                             // Start index of the segment in the path
  Int endIndex;                               // End index of the segment in the path

public:

  // Constructor
  NavigationPathSegment();

  // Destructor
  virtual ~NavigationPathSegment();

  // Getters and setters
  Int getEndIndex() const {
    return endIndex;
  }

  void setEndIndex(Int endIndex) {
    this->endIndex = endIndex;
  }

  NavigationPath* getPath() const {
    return path;
  }

  void setPath(NavigationPath* path) {
    this->path = path;
  }

  Int getStartIndex() const {
    return startIndex;
  }

  void setStartIndex(Int startIndex) {
    this->startIndex = startIndex;
  }

  NavigationPathVisualization* getVisualization() {
    return visualization;
  }

  void setVisualization(NavigationPathVisualization* visualization) {
    this->visualization = visualization;
  }
};

} /* namespace GEODISCOVERER */
#endif /* NAVIGATIONPATHSEGMENT_H_ */
