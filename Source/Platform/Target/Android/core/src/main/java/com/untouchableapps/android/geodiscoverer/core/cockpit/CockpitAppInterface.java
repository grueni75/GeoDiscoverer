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

package com.untouchableapps.android.geodiscoverer.core.cockpit;

/** Interface used by the cockpit engine on the cockpit apps */
public interface CockpitAppInterface {
  
  enum AlertType { offRoute, newTurn };
  
  /** Prepares an update of the information shown in cockpit (e.g., create new screen drawing) */
  void update(CockpitInfos infos);
  
  /** Updates the cockpit with the latest infos (e.g., show screen drawing from last update) */
  void inform();
  
  /** Puts the cockpit into focus (e.g., brings the cockpit screen in the foreground) */
  void focus();
  
  /** Alerts the user (e.g., vibrates the cockpit) */
  void alert(AlertType type, boolean repeated);

  /** Starts the cockpit app */
  void start();
  
  /** Stops the cockpit app */
  void stop();

}
