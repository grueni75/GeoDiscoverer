//============================================================================
// Name        : NavigationPathSegment.h
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
