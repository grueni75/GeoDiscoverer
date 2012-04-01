//============================================================================
// Name        : FloatingPoint.cpp
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


#include <Core.h>

namespace GEODISCOVERER {

FloatingPoint::FloatingPoint() {
  // TODO Auto-generated constructor stub

}

FloatingPoint::~FloatingPoint() {
  // TODO Auto-generated destructor stub
}

// Computes the constants c0-c1 of the equation c0*x+c1
std::vector<double> FloatingPoint::solveZEqualsC0XPlusC1(std::vector<double> &x, std::vector<double> &z) {
  std::vector<double> c;
  c.resize(2);
  double t;
  if (x.size()!=2) {
    FATAL("two samples for x are required to solve z=c0*x+c1",NULL);
    return c;
  }
  if (z.size()!=2) {
    FATAL("two samples for z are required to solve z=c0*x+c1",NULL);
    return c;
  }
  for (int i=0;i<2;i++) {
    DEBUG("x[%d]=%.2f z[%d]=%.2f",i,x[i],i,z[i]);
  }

  // Compute differences between factors
  double x1x0=x[1]-x[0];
  double z1z0=z[1]-z[0];

  // Compute c0
  if (x1x0==0.0) {
    ERROR("can not compute c[1] (divisor is zero)",NULL);
    return std::vector<double>();
  }
  c[0]=z1z0/x1x0;
  DEBUG("c[0]=%e",c[0]);

  // Compute c1
  c[1]=z[0]-c[0]*x[0];
  DEBUG("c[1]=%e",c[1]);
  return c;
}

// Computes the constants c0-c2 of the equation c0*x+c1*y+c2
std::vector<double> FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(std::vector<double> &x, std::vector<double> &y, std::vector<double> &z) {
  std::vector<double> c;
  c.resize(3);
  double t;
  if (x.size()!=3) {
    FATAL("three samples for x are required to solve z=c0*x+c1*y+c2",NULL);
    return c;
  }
  if (y.size()!=3) {
    FATAL("three samples for y are required to solve z=c0*x+c1*y+c2",NULL);
    return c;
  }
  if (z.size()!=3) {
    FATAL("three samples for z are required to solve z=c0*x+c1*y+c2",NULL);
    return c;
  }
  /*for (int i=0;i<3;i++) {
    DEBUG("x[%d]=%.2f y[%d]=%.2f z[%d]=%.2f",i,x[i],i,y[i],i,z[i]);
  }*/

  // Compute differences between factors
  double x2x0=x[2]-x[0];
  double x1x0=x[1]-x[0];
  double y2y0=y[2]-y[0];
  double y1y0=y[1]-y[0];
  double z1z0=z[1]-z[0];
  double z2z0=z[2]-z[0];

  // Compute c1
  t=x2x0*y1y0-x1x0*y2y0;
  if (t==0.0) {
    ERROR("can not compute c[1] (divisor is zero)",NULL);
    return std::vector<double>();
  }
  c[1]=(x2x0*z1z0-x1x0*z2z0)/t;
  //DEBUG("c[1]=%e",c[1]);

  // Compute c0
  if ((x2x0==0.0)&&(x1x0==0.0)) {
    ERROR("can not compute c[0] (divisor is zero)",NULL);
    return std::vector<double>();
  }
  if (x2x0==0.0) {
    c[0]=(z1z0-c[1]*y1y0)/x1x0;
  } else {
    c[0]=(z2z0-c[1]*y2y0)/x2x0;
  }
  //DEBUG("c[0]=%e",c[0]);

  // Compute c2
  c[2]=z[0]-c[0]*x[0]-c[1]*y[0];
  //DEBUG("c[2]=%e",c[2]);
  return c;
}

/*
  z=c1*x+cx2*y+c3
def solve_equation(l,mode):

  lng=[]
  lng.append(l[0].lng)
  lng.append(l[1].lng)
  lng.append(l[2].lng)
  lat=[]
  lat.append(l[0].lat)
  lat.append(l[1].lat)
  lat.append(l[2].lat)
  a=[]
  if mode=="x":
    a.append(l[0].x)
    a.append(l[1].x)
    a.append(l[2].x)
  else:
    a.append(l[0].y)
    a.append(l[1].y)
    a.append(l[2].y)
  u1=a[0]-lng[0]/lng[1]*a[1]
  v1=lat[0]-lng[0]*lat[1]/lng[1]
  w1=1-lng[0]/lng[1]
  u2=a[0]-lng[0]/lng[2]*a[2]
  v2=lat[0]-lng[0]*lat[2]/lng[2]
  w2=1-lng[0]/lng[2]
  t=w1*v2-w2*v1
  s=u1*v2-u2*v1
  if (t==0) and (s<>0):
    return (0,0,0,0)
  else:
    c3=s/t
    c2=(u1-w1*c3)/v1
    c1=(a[0]-c2*lat[0]-c3)/lng[0]
    return (1,c1,c2,c3)
 */

  // Converts degree to rad
  double FloatingPoint::degree2rad(double degree) {
    return degree*M_PI/180.0;
  }

  // Converts rad to degree
  double FloatingPoint::rad2degree(double rad) {
    return rad*180.0/M_PI;
  }

  // Compute the angle of a orthogonal triangle
  double FloatingPoint::computeAngle(double adjacent, double opposite) {
    // tan alpha = opposite / adjacent
    double alpha;
    if (adjacent==0) {
      if (opposite>=0) {
        alpha=M_PI/2;
      } else {
        alpha=M_PI+M_PI/2;
      }
    } else {
      alpha=atan(fabs(opposite/adjacent));
      if (adjacent<0) {
        if (opposite<0) {
          alpha+=M_PI;
        } else {
          alpha=M_PI-alpha;
        }
      } else {
        if (opposite<0) {
          alpha=2*M_PI-alpha;
        } else {
          alpha+=0;
        }
      }
    }
    return alpha;
  }

  // Bilinear approximation
  /*
   * Copied from geoid.c -- ECEF to WGS84 conversions, including ellipsoid-to-MSL height
   *
   * Geoid separation code by Oleg Gusev, from data by Peter Dana.
   * ECEF conversion by Rob Janssen.
   *
   * This file is Copyright (c) 2010 by the GPSD project
   * BSD terms apply: see the file COPYING in the distribution root for details.
   */
  double FloatingPoint::bilinear(double x1, double y1, double x2, double y2, double x, double y, double z11, double z12, double z21, double z22) {

    double delta;

    if (y1 == y2 && x1 == x2)
      return (z11);
    if (y1 == y2 && x1 != x2)
      return (z22 * (x - x1) + z11 * (x2 - x)) / (x2 - x1);
    if (x1 == x2 && y1 != y2)
      return (z22 * (y - y1) + z11 * (y2 - y)) / (y2 - y1);

    delta = (y2 - y1) * (x2 - x1);

    return (z22 * (y - y1) * (x - x1) + z12 * (y2 - y) * (x - x1) + z21 * (y - y1) * (x2 - x) + z11 * (y2 - y) * (x2 - x)) / delta;
  }

}
