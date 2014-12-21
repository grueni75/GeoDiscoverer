//============================================================================
// Name        : CockpitAppInterface.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer;

/** Interface used by the cockpit engine on the cockpit apps */
public interface CockpitAppInterface {
  
  enum AlertType { offRoute, newTurn };
  
  /** Prepares an update of the information shown in cockpit (e.g., create new screen drawing) */
  public void update(CockpitInfos infos);
  
  /** Updates the cockpit with the latest infos (e.g., show screen drawing from last update) */
  public void inform();
  
  /** Puts the cockpit into focus (e.g., brings the cockpit screen in the foreground) */
  public void focus();
  
  /** Alerts the user (e.g., vibrates the cockpit) */
  public void alert(AlertType type);

  /** Starts the cockpit app */
  public void start();
  
  /** Stops the cockpit app */
  public void stop();

}
