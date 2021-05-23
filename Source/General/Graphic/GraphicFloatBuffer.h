//============================================================================
// Name        : GraphicFloatBuffer.h
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

#include <Screen.h>

#ifndef GRAPHICFLOATBUFFER_H_
#define GRAPHICFLOATBUFFER_H_

namespace GEODISCOVERER {

class GraphicFloatBuffer {

protected:

  Screen *screen;                                    // Screen this point buffer belongs to
  Float *points;                                     // Pointer to the points data
  Int numberOfPoints;                                // Number of points that can be stored in the buffer
  Int insertPos;                                     // The position where to insert a new point
  GraphicBufferInfo buffer;                          // ID of the vertex buffer to use
  bool bufferOutdated;                               // Indicates that the buffer must be recreated

public:

  // Constructor
  GraphicFloatBuffer(Screen *screen, Int numberOfPoints);

  // Destructor
  virtual ~GraphicFloatBuffer();

  // Adds one point to the internal array
  bool addPoint(Float point);

  // Adds a list of points to the internal array
  bool addPoints(std::list<Float> *points);

  // Removes all points
  void reset();

  // Updates the buffer contents
  void updateBuffer();

  // Invalidates any textures or buffers
  void invalidate();

  // Getters and setters
  Int getSize() const
  {
    return insertPos;
  }

  bool getIsFull() const
  {
    return (insertPos==numberOfPoints);
  }

  Float getPoint(Int pos) {
    return points[pos];
  }

  GraphicBufferInfo getBuffer() const
  {
      return buffer;
  }
};

}

#endif /* GRAPHICFLOATBUFFER_H_ */
