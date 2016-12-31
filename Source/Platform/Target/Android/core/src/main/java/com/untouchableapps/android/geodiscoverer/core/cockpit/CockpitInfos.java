//============================================================================
// Name        : CockpitInfos.java
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


package com.untouchableapps.android.geodiscoverer.core.cockpit;

/** Holds all infos that the cockpit apps can use */
public class CockpitInfos {
  
  /** Direction of current movement */
  public String locationBearing;

  /** Current speed in m/s */
  public String locationSpeed;

  /** Bearing towards the given target */
  public String targetBearing;

  /** Remaining distance to the target */
  public String targetDistance;

  /** Remaining travel time to the target */
  public String targetDuration;

  /** Remaining distance to next turn */
  public String turnDistance;

  /** Angle by which the turn will change the direction */
  public String turnAngle;

  /** Indicates if location is off route */
  public boolean offRoute;
  
  /** Remaining distance to route */
  public String routeDistance;

}
