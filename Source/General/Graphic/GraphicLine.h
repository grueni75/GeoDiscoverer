//============================================================================
// Name        : GraphicLine.h
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

#include <GraphicPointBuffer.h>
#include <GraphicPrimitive.h>

#ifndef GRAPHICLINE_H_
#define GRAPHICLINE_H_

namespace GEODISCOVERER {

class GraphicLine : public GraphicPrimitive {

protected:

  std::list<GraphicPointBuffer*> segments;           // List of segments the line consists of
  Int numberOfTrianglesOtherSegments;                // Number of triangles in all other segments
  GraphicPointBuffer *currentSegment;                // The current segment where points are added to
  Short width;                                       // Width of the line
  Int valuesPerTriangle;                             // Number of values to represent a triangle
  Int cutEnabled;                                    // If set: line will be cutted such that it is within the area defined by x,y,cutWidth,cutHeight
  Int cutWidth;                                      // Cut the line that it is within the given width
  Int cutHeight;                                     // Cut the line that it is within the given height

  // Checks if an overflow has occured
  bool pointIsValid(double x, double y);

  // Updates the point such that it lies within the cut range
  void cutPoint(Short &x, Short &y, double m, double ys);

public:

  GraphicLine(Screen *screen, Int numberOfStrokes, Short width);
  virtual ~GraphicLine();

  // Adds two points to the line
  void addStroke(Short x0, Short y0, Short x1, Short y1);

  // Adds one point to the internal array
  void addPoint(Short x, Short y);

  // Frees all memories
  void deinit();

  // Draws the line
  void draw();

  // Recreates any textures or buffers
  virtual void invalidate();

  // Reduce the number of point buffers required
  virtual void optimize();

  // Getters and setters
  void setCutEnabled(Int cutEnabled)
  {
      this->cutEnabled = cutEnabled;
  }

  void setCutHeight(Int cutHeight)
  {
      this->cutHeight = cutHeight;
  }

  void setCutWidth(Int cutWidth)
  {
      this->cutWidth = cutWidth;
  }

  Int getCutEnabled() const {
    return cutEnabled;
  }

  Int getCutHeight() const {
    return cutHeight;
  }

  Int getCutWidth() const {
    return cutWidth;
  }

  std::list<GraphicPointBuffer*> *getSegments() {
    return &segments;
  }

  Short getWidth() const {
    return width;
  }
};

}

#endif /* GRAPHICLINE_H_ */
