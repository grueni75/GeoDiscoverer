//============================================================================
// Name        : GraphicLineSegment.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
  bool addPoints(std::list<GraphicPoint> points);

  // Removes all points
  void reset();

  // Uses the stored points to draw triangles
  void drawAsTriangles();

  // Uses the stored points to draw textured triangles
  void drawAsTexturedTriangles(GraphicTextureInfo textureInfo, GraphicPointBuffer *textureCoordinates);

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
