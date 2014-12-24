//============================================================================
// Name        : WidgetPosition.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef WIDGETPOSITION_H_
#define WIDGETPOSITION_H_

namespace GEODISCOVERER {

class WidgetPosition {

  // Reference screen diagonal for this position
  double refScreenDiagonal;

  // Portrait coordinates of the widget
  double portraitX, portraitY;
  Int portraitZ;

  // Landscape coordinates of the widget
  double landscapeX, landscapeY;
  Int landscapeZ;

public:

  // Constructor
  WidgetPosition();

  // Destructor
  virtual ~WidgetPosition();

  // Getters and setters
  double getLandscapeX() const {
    return landscapeX;
  }

  void setLandscapeX(double landscapeX) {
    this->landscapeX = landscapeX;
  }

  double getLandscapeY() const {
    return landscapeY;
  }

  void setLandscapeY(double landscapeY) {
    this->landscapeY = landscapeY;
  }

  double getLandscapeZ() const {
    return landscapeZ;
  }

  void setLandscapeZ(double landscapeZ) {
    this->landscapeZ = landscapeZ;
  }

  double getPortraitX() const {
    return portraitX;
  }

  void setPortraitX(double portraitX) {
    this->portraitX = portraitX;
  }

  double getPortraitY() const {
    return portraitY;
  }

  void setPortraitY(double portraitY) {
    this->portraitY = portraitY;
  }

  double getPortraitZ() const {
    return portraitZ;
  }

  void setPortraitZ(double portraitZ) {
    this->portraitZ = portraitZ;
  }

  double getRefScreenDiagonal() const {
    return refScreenDiagonal;
  }

  void setRefScreenDiagonal(double refScreenDiagonal) {
    this->refScreenDiagonal = refScreenDiagonal;
  }
};

} /* namespace GEODISCOVERER */

#endif /* WIDGETPOSITION_H_ */
