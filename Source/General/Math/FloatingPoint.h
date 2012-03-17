//============================================================================
// Name        : FloatingPoint.h
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


#ifndef FLOATINGPOINT_H_
#define FLOATINGPOINT_H_

namespace GEODISCOVERER {

class FloatingPoint {
public:
  FloatingPoint();
  virtual ~FloatingPoint();

  // Solve z=c1*x+c2*y+c3
  static std::vector<double> solveZEqualsC1XPlusC2YPlusC3(std::vector<double> &x,std::vector<double> &y,std::vector<double> &z);

  // Compute the angle of a orthogonal triangle
  static double computeAngle(double adjacent, double opposite);

  // Converts degree to rad
  static double degree2rad(double degree);

  // Converts rad to degree
  static double rad2degree(double rad);

  // Bilinear approximation
  static double bilinear(double x1, double y1, double x2, double y2, double x, double y, double z11, double z12, double z21, double z22);

};

}

#endif /* FLOATINGPOINT_H_ */
