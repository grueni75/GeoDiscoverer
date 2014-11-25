//============================================================================
// Name        : CockpitAppInterface.java
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
