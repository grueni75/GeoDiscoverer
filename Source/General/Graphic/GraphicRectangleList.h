//============================================================================
// Name        : GraphicRectangleList.h
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

#include <GraphicRectangleListSegment.h>
#include <GraphicPrimitive.h>

#ifndef GRAPHICRECTANGLELIST_H_
#define GRAPHICRECTANGLELIST_H_

namespace GEODISCOVERER {

class GraphicRectangleList : public GraphicPrimitive {

protected:

  std::list<GraphicRectangleListSegment*> segments;   // List of segments the rectangle list consists of
  bool enableTimeColoring;                            // Enables coloring of vertices depending on time
  Int numberOfRectanglesOtherSegments;                // Number of rectangle in all other segments
  GraphicRectangleListSegment *currentSegment;        // The current segment where points are added to
  Int cutEnabled;                                     // If set: line will be cutted such that it is within the area defined by x,y,cutWidth,cutHeight
  Int cutWidth;                                       // Cut the line that it is within the given width
  Int cutHeight;                                      // Cut the line that it is within the given height
  double radius;                                      // Radius to the edge points of the rectangle
  double distanceToCenter;                            // Distance from the icon center to the rectangle center
  double angleToCenter;                               // Angle from the icon center to the rectangle center

public:

  // Constructor
  GraphicRectangleList(Screen *screen, Int numberOfRectangles, bool enableTimeColoring);

  // Destructor
  virtual ~GraphicRectangleList();

  // Adds a new rectangle (from center point and angle)
  void addRectangle(double x, double y, double angle, Float timeOffset=0);

  // Adds a new rectangle (from corner points)
  void addRectangle(Short x[4], Short y[4], Float timeOffset=0);

  // Frees all memories
  void deinit();

  // Draws the rectangles
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

  void setParameter(double radius, double distanceToCenter, double angleToCenter)
  {
      this->radius = radius;
      this->distanceToCenter=distanceToCenter;
      this->angleToCenter=angleToCenter;
  }

  double getAngleToCenter() const {
    return angleToCenter;
  }

  double getDistanceToCenter() const {
    return distanceToCenter;
  }

  double getRadius() const {
    return radius;
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

  std::list<GraphicRectangleListSegment*> *getSegments() {
    return &segments;
  }
};

}

#endif /* GRAPHICRECTANGLELIST_H_ */
