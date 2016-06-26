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
