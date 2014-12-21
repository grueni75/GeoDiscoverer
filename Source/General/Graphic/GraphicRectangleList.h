//============================================================================
// Name        : GraphicRectangleList.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef GRAPHICRECTANGLELIST_H_
#define GRAPHICRECTANGLELIST_H_

namespace GEODISCOVERER {

class GraphicRectangleList : public GraphicPrimitive {

protected:

  std::list<GraphicRectangleListSegment*> segments;   // List of segments the rectangle list consists of
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
  GraphicRectangleList(Int numberOfRectangles);

  // Destructor
  virtual ~GraphicRectangleList();

  // Adds a new rectangle
  void addRectangle(double x, double y, double angle);

  // Frees all memories
  void deinit();

  // Draws the rectangles
  void draw(Screen *screen);

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
};

}

#endif /* GRAPHICRECTANGLELIST_H_ */
