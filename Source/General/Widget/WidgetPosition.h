//============================================================================
// Name        : WidgetPosition.h
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

#ifndef WIDGETPOSITION_H_
#define WIDGETPOSITION_H_

namespace GEODISCOVERER {

class WidgetPosition {

  // Reference screen diagonal for this position
  double refScreenDiagonal;

  // Portrait coordinates of the widget
  double portraitX, portraitY;
  double portraitXHidden, portraitYHidden;
  Int portraitZ;

  // Landscape coordinates of the widget
  double landscapeX, landscapeY;
  double landscapeXHidden, landscapeYHidden;
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

  double getLandscapeXHidden() const {
    return landscapeXHidden;
  }

  void setLandscapeXHidden(double landscapeXHidden) {
    this->landscapeXHidden = landscapeXHidden;
  }

  double getLandscapeYHidden() const {
    return landscapeYHidden;
  }

  void setLandscapeYHidden(double landscapeYHidden) {
    this->landscapeYHidden = landscapeYHidden;
  }

  double getPortraitXHidden() const {
    return portraitXHidden;
  }

  void setPortraitXHidden(double portraitXHidden) {
    this->portraitXHidden = portraitXHidden;
  }

  double getPortraitYHidden() const {
    return portraitYHidden;
  }

  void setPortraitYHidden(double portraitYHidden) {
    this->portraitYHidden = portraitYHidden;
  }
};

} /* namespace GEODISCOVERER */

#endif /* WIDGETPOSITION_H_ */
