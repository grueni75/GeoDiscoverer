//============================================================================
// Name        : CockpitAppVibration.java
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

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitAppInterface;

public class CockpitAppVibration implements CockpitAppInterface {

  // Context
  Context context;
  
  // Vibrator system service
  Vibrator vibrator;

  /** Gets the vibrator in legacy way */
  @SuppressWarnings("deprecation")
  private Vibrator getLegacyVibrator() {
    return (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
  }

  /** Constructor */
  public CockpitAppVibration(Context context) {
    super();
    
    // Remember important variables
    this.context = context;

    // Set the vibrator
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      vibrator =  ((VibratorManager)context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE)).getDefaultVibrator();
    } else {
      vibrator = getLegacyVibrator();
    }
  }

  /** Starts the app */
  public void start() {
  }

  /** Brings the app to foreground */
  public void focus() {
  }
  
  /** Shows the latest info */
  public void inform() {
  }

  /** Legacy vibrate function */
  @SuppressWarnings("deprecation")
  public void legacyVibrate(long[] pattern) {
    vibrator.vibrate(pattern, -1);
  }

  /** Inform the user via vibration */
  public void alert(AlertType type, boolean repeated) {
    long[] pattern = { 0, 500, 500, 500 };
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      vibrator.vibrate(VibrationEffect.createWaveform(pattern, -1));
    } else {
      legacyVibrate(pattern);
    }
  }
  
  /** Updates the latest info */
  public void update(CockpitInfos infos) {
  }
  
  /** Stops the app */
  public void stop() {
  }

}
