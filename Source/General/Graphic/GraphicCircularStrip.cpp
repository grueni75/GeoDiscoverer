//============================================================================
// Name        : GraphicCircularStrip.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2019 Matthias Gruenewald
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

#include <Core.h>
#include <GraphicCircularStrip.h>
#include <FloatingPoint.h>

namespace GEODISCOVERER {

// Constructors
GraphicCircularStrip::GraphicCircularStrip(Screen *screen) : GraphicRectangle(screen)
{
  type=GraphicTypeCircularStrip;
  coordinatesOutdated=true;
  radius=0;
  angle=0;
  triangleCoordinates=NULL;
  textureCoordinates=NULL;
  inverse=false;
}

// Destructor
GraphicCircularStrip::~GraphicCircularStrip() {
  if (textureCoordinates) delete textureCoordinates;
  if (triangleCoordinates) delete triangleCoordinates;
}

// Called when the rectangle must be drawn
void GraphicCircularStrip::draw(TimestampInMicroseconds t) {

  // Only draw if we have data
  if (triangleCoordinates==NULL)
    return;

  // Set color
  screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),getColor().getAlpha());

  // Draw the strip
  screen->startObject();
  screen->translate(x,y,0);
  triangleCoordinates->drawAsTexturedTriangles(texture,textureCoordinates,true);
  screen->endObject();
}

// Recreates any textures or buffers
void GraphicCircularStrip::invalidate() {
  if (triangleCoordinates) triangleCoordinates->invalidate();
  if (textureCoordinates) textureCoordinates->invalidate();
}

// Lets the primitive work (e.g., animation)
bool GraphicCircularStrip::work(TimestampInMicroseconds currentTime) {
  bool changed=false;

  // Do we need to recalculate the buffers?
  if (coordinatesOutdated) {

    // Create the triangles the strip consists of
    Int numberOfPoints = getIconWidth()/2;
    if (triangleCoordinates) delete triangleCoordinates;
    triangleCoordinates=new GraphicPointBuffer(screen,(numberOfPoints-1)*6);
    if (textureCoordinates) delete textureCoordinates;
    textureCoordinates=new GraphicPointBuffer(screen,(numberOfPoints-1)*6);
    double textureWidth = round((double)getIconWidth()/(double)getWidth()*32767.0);
    double textureTop = 32767.0;
    double textureBottom = (getHeight()-getIconHeight())*(32767.0/(double)getHeight());
    if (inverse) {
      textureTop = textureBottom;
      textureBottom = 32767.0;
    }
    double textureStep = textureWidth/((double)(numberOfPoints-1));
    double thickness = getIconHeight();
    double totalAngle = ((double)getIconWidth())/radius;
    double alphaStep = -totalAngle/((double)(numberOfPoints-1));
    double alpha = FloatingPoint::degree2rad(this->angle)+totalAngle/2.0;
    if (inverse) {
      alphaStep = -alphaStep;
      alpha -= totalAngle;
    }
    double textX = 0;
    double coordX, coordY, r;
    double prevTextX, prevCoordX, prevCoordY;
    Short x,y;    
    for (Int i=1;i<numberOfPoints;i++) {

      //DEBUG("i=%d alpha=%f",i,FloatingPoint::rad2degree(alpha));
      
      // Lower left point
      r = radius-thickness/2;
      coordX = r*cos(alpha);
      coordY = r*sin(alpha);
      triangleCoordinates->addPoint(round(coordX),round(coordY));
      textureCoordinates->addPoint(round(textX),textureTop);

      // Upper left point
      r = radius+thickness/2;
      coordX = r*cos(alpha);
      coordY = r*sin(alpha);
      triangleCoordinates->addPoint(round(coordX),round(coordY));
      textureCoordinates->addPoint(round(textX),textureBottom);
      prevTextX = textX;
      prevCoordX = coordX;
      prevCoordY = coordY;

      // Next step
      alpha += alphaStep;
      textX += textureStep;

      // Lower right point (end of first triangle and start of second)
      r = (radius-thickness/2);
      coordX = r*cos(alpha);
      coordY = r*sin(alpha);
      triangleCoordinates->addPoint(round(coordX),round(coordY));
      textureCoordinates->addPoint(round(textX),textureTop);
      triangleCoordinates->addPoint(round(coordX),round(coordY));
      textureCoordinates->addPoint(round(textX),textureTop);

      // Upper left point (re-used from previous step)
      triangleCoordinates->addPoint(round(prevCoordX),round(prevCoordY));
      textureCoordinates->addPoint(round(prevTextX),textureBottom);

      // Upper right point
      r = (radius+thickness/2);
      coordX = r*cos(alpha);
      coordY = r*sin(alpha);
      triangleCoordinates->addPoint(round(coordX),round(coordY));
      textureCoordinates->addPoint(round(textX),textureBottom);
    }
    changed=true;
    coordinatesOutdated=false;
  }

  return changed;
}


}
