//============================================================================
// Name        : CockpitAppVoice.java
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

package com.untouchableapps.android.geodiscoverer.logic.cockpit;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitAppInterface;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitInfos;

public class CockpitAppTandemTracker implements CockpitAppInterface {

  // Context
  Context context;

  // Cockpit engine
  CockpitEngine cockpitEngine;

  /** Constructor */
  public CockpitAppTandemTracker(Context context, CockpitEngine cockpitEngine) {
    super();
        
    // Init parameters
    //minDurationBetweenOffRouteAlerts = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "minDurationBetweenOffRouteAlerts")) *  1000;

    // Remember variables
    this.context = context;
    this.cockpitEngine = cockpitEngine;
  }

  /** Prepare voice output */
  public void start() {
  }

  /** Not necessary */
  public void focus() {
  }
  
  /** Tell what is happening */
  public void inform() {
  }  

  /** Not necessary */
  public void alert(AlertType type, boolean repeated) {
  }

  /** Prepare the text to say */
  public void update(CockpitInfos cockpitInfos) {

    // Construct navigation info
    String value="";
    value+=cockpitInfos.locationBearing + ";";
    value+=cockpitInfos.locationSpeed + ";";
    value+=cockpitInfos.trackLength + ";";
    value+=cockpitInfos.targetBearing + ";";
    value+=cockpitInfos.targetDistance + ";";
    value+=cockpitInfos.targetDuration + ";";
    value+=cockpitInfos.turnDistance + ";";
    value+=cockpitInfos.turnAngle + ";";
    value+=cockpitInfos.offRoute ? "true;" : "false;";
    value+=cockpitInfos.routeDistance + ";";
    value+=cockpitInfos.nearestNavigationPointBearing + ";";
    value+=cockpitInfos.nearestNavigationPointDistance;

    // And send it
    Intent result = new Intent();
    result.setAction("org.mg.tandemtracker.update_navigation_info");
    result.putExtra("navigationInfo", value);
    result.setComponent(new ComponentName("org.mg.tandemtracker", "org.mg.tandemtracker.BackgroundService"));
    ComponentName c = context.startForegroundService(result);

    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("navigationInfo=%s",value));
  }
  
  /** Clean up everything */
  public void stop() {
  }

}
