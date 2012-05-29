//============================================================================
// Name        : GraphicLineSegment.h
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


#ifndef GRAPHICPOINTBUFFER_H_
#define GRAPHICPOINTBUFFER_H_

namespace GEODISCOVERER {

class GraphicPointBuffer {

protected:

  Short *points;                                     // Pointer to the points data
  Int numberOfPoints;                                // Number of points that can be stored in the buffer
  Int insertPos;                                     // The position where to insert a new point
  GraphicBufferInfo buffer;                          // ID of the vertex buffer to use
  bool bufferOutdated;                               // Indicates that the buffer must be recreated
  static std::list<GraphicBufferInfo> unusedBuffers; // List of unused buffers

public:

  // Constructor
  GraphicPointBuffer(Int numberOfPoints);

  // Destructor
  virtual ~GraphicPointBuffer();

  // Adds one point to the internal array
  bool addPoint(Short x, Short y);

  // Uses the stored points to draw triangles
  void drawAsTriangles(Screen *screen);

  // Uses the stored points to draw textured triangles
  void drawAsTexturedTriangles(Screen *screen, GraphicTextureInfo textureInfo, GraphicPointBuffer *textureCoordinates);

  // Frees all used buffer objects
  static void destroyBuffers();

  // Updates the buffer contents
  void updateBuffer(Screen *screen);

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
