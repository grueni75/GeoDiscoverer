//============================================================================
// Name        : GraphicPosition.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

GraphicPosition::GraphicPosition() {
  set(0,0,1.0,0.0);
  lastUserModification=0;
}

GraphicPosition::~GraphicPosition() {
  // TODO Auto-generated destructor stub
}

// Changes the rotation
void GraphicPosition::rotate(double degree) {
  valueAngle+=degree;
  while (valueAngle>=360.0) {
    valueAngle=valueAngle-360.0;
  }
  while (valueAngle<0.0) {
    valueAngle=360.0+valueAngle;
  }
  //DEBUG("degree=%f valueAngle=%f",degree,valueAngle);
  changed=true;
}

// Changes the zoom
void GraphicPosition::zoom(double scale) {
  //std::cout << "stepZoom=" << stepZoom.toString() << std::endl;
  //std::cout << "stepZoom=" << stepZoom.toDouble() << std::endl;
  //valueZoom+=FixedPoint(offset)*stepZoom;
  valueZoom*=scale;
  //std::cout << "valueZoom=" << valueZoom.toString() << std::endl;
  changed=true;
}

// Pans the position
void GraphicPosition::pan(Int xi, Int yi) {

  // Compute the displacement depending on the zoom
  //DEBUG("before zoom adaption: x=%d y=%d",xi,yi);
  //DEBUG("zoomValue=%f",valueZoom);
  double xd=(double)xi/valueZoom;
  double yd=(double)yi/valueZoom;
  //DEBUG("after zoom adaption: x=%f y=%f",xd,yd);

  // Compute the new position
  double alpha=getAngleRad();
  double beta=FloatingPoint::computeAngle(xi,yi);
  double gamma=beta-alpha;
  double l=xd*xd+yd*yd;
  l=sqrt(l);
  double xt=l*cos(gamma);
  double yt=l*sin(gamma);
  //DEBUG("x=%d y=%d xt=%.4f yt=%.4f l=%.4f alpha=%.4f beta=%.4f gamma=%.4f",x,y,xt.toDouble(),yt.toDouble(),l.toDouble(),alpha.toDouble(),beta.toDouble(),gamma.toDouble());
  valueX += xt;
  valueY += yt;
  changed=true;
  //DEBUG("valueX=%.2f valueY=%.2f",valueX,valueY);
}

// Returns the angle in radiant
double GraphicPosition::getAngleRad() {
  return FloatingPoint::degree2rad(getAngle());
}

// Sets the position
void GraphicPosition::set(Int valueX, Int valueY, double valueZoom, double valueAngle) {
  this->valueAngle=valueAngle;
  this->valueZoom=valueZoom;
  this->valueX=valueX;
  this->valueY=valueY;
  changed=true;
}

// Operators
bool GraphicPosition::operator==(const GraphicPosition &rhs)
{
  if ((getAngle()==rhs.getAngle())&&(getZoom()==rhs.getZoom())&&(getX()==rhs.getX())
  &&(getY()==rhs.getY()))
    return true;
  else
    return false;
}
bool GraphicPosition::operator!=(const GraphicPosition &rhs)
{
  return (!(*this==rhs));

}

}
