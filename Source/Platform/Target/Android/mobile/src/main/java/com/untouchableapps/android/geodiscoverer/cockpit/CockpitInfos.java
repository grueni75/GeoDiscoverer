//============================================================================
// Name        : CockpitInfos.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


package com.untouchableapps.android.geodiscoverer.cockpit;

/** Holds all infos that the cockpit apps can use */
public class CockpitInfos {
  
  /** Direction of current movement */
  String locationBearing;

  /** Current speed in m/s */
  String locationSpeed;

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
  
  /** Remaining distance to route */
  String routeDistance;

}
