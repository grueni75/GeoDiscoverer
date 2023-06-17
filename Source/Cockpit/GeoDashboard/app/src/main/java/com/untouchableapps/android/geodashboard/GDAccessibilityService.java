//============================================================================
// Name        : GDAccessibilityService.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

package com.untouchableapps.android.geodashboard;

import android.accessibilityservice.AccessibilityService;

import android.content.Intent;
import android.util.Log;
import android.view.accessibility.AccessibilityEvent;

public class GDAccessibilityService extends AccessibilityService {

  @Override
  public void onCreate() {
    super.onCreate();
  }

  @Override
  protected void onServiceConnected() {
    super.onServiceConnected();
    Log.d("GeoDashboard", "onServiceConnected called");
  }

  @Override
  public void onInterrupt() {
    Log.d("GeoDashboard", "onInterrupt called");
  }

  @Override
  public void onAccessibilityEvent(AccessibilityEvent event) {
    Log.d("GeoDashboard", "onAccessibilityEvent called");
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    if ((intent!=null)&&(intent.getAction()!=null)&&(intent.getAction()=="showPowerMenu")) {
        Log.d("GeoDashboard", "showing power dialog");
        performGlobalAction(GLOBAL_ACTION_POWER_DIALOG);
    }
    return super.onStartCommand(intent, flags, startId);
  }
}
