//============================================================================
// Name        : CockpitInfos.java
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


package com.untouchableapps.android.geodiscoverer;

/** Holds all infos that the cockpit apps can use */
public class CockpitInfos {
  
  /** Direction of current movement */
  String locationBearing;

  /** Bearing towards the given target */
  String targetBearing;

  /** Remaining distance to the target */
  String targetDistance;

  /** Remaining travel time to the target */
  String targetDuration;

  /** Remaining distance to next turn */
  String turnDistance;

  /** Angle by which the turn will change the direction */
  String turnAngle;

  /** Indicates if location is off route */
  boolean offRoute;
  
}
