//============================================================================
// Name        : GraphicLineSegment.h
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


#ifndef GRAPHICPOINTBUFFER_H_
#define GRAPHICPOINTBUFFER_H_

namespace GEODISCOVERER {

class GraphicPointBuffer {

protected:

  Screen *screen;                                    // Screen this point buffer belongs to
  Short *points;                                     // Pointer to the points data
  Int numberOfPoints;                                // Number of points that can be stored in the buffer
  Int insertPos;                                     // The position where to insert a new point
  GraphicBufferInfo buffer;                          // ID of the vertex buffer to use
  bool bufferOutdated;                               // Indicates that the buffer must be recreated

public:

  // Constructor
  GraphicPointBuffer(Screen *screen, Int numberOfPoints);

  // Destructor
  virtual ~GraphicPointBuffer();

  // Adds one point to the internal array
  bool addPoint(Short x, Short y);

  // Adds one point to the internal array
  bool addPoint(GraphicPoint point);

  // Adds a list of points to the internal array
  bool addPoints(std::list<GraphicPoint> *points);

  // Removes all points
  void reset();

  // Uses the stored points to draw triangles
  void drawAsTriangles();

  // Uses the stored points to draw textured triangles
  void drawAsTexturedTriangles(GraphicTextureInfo textureInfo, GraphicPointBuffer *textureCoordinates, boolean normalizeTextureCoordinates=false);

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

  void getPoint(Int pos, Short &x, Short &y) {
    x=points[pos*2];
    y=points[pos*2+1];
  }

  GraphicBufferInfo getBuffer() const
  {
      return buffer;
  }
};

}

#endif /* GRAPHICPOINTBUFFER_H_ */
