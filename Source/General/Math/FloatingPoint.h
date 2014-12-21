//============================================================================
// Name        : FloatingPoint.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef FLOATINGPOINT_H_
#define FLOATINGPOINT_H_

namespace GEODISCOVERER {

class FloatingPoint {
public:
  FloatingPoint();
  virtual ~FloatingPoint();

  // Solve z=c0*x+c1
  static std::vector<double> solveZEqualsC0XPlusC1(std::vector<double> &x, std::vector<double> &z);

  // Solve z=c0*x+c1*y+c2
  static std::vector<double> solveZEqualsC0XPlusC1YPlusC2(std::vector<double> &x,std::vector<double> &y,std::vector<double> &z);

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
