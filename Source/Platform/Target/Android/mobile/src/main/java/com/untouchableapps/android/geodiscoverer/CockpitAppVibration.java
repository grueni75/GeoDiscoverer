//============================================================================
// Name        : CockpitAppVibration.java
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

import android.content.Context;
import android.os.Vibrator;

public class CockpitAppVibration implements CockpitAppInterface {

  // Context
  Context context;
  
  // Vibrator system service
  Vibrator vibrator;
  
  /** Constructor */
  public CockpitAppVibration(Context context) {
    super();
    
    // Remember important variables
    this.context = context;
    vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);

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

  /** Inform the user via vibration */
  public void alert(AlertType type, boolean repeated) {
    long[] pattern = { 0, 500, 500, 500 };
    vibrator.vibrate(pattern, -1);
  }  
  
  /** Updates the latest info */
  public void update(CockpitInfos infos) {
  }
  
  /** Stops the app */
  public void stop() {
  }

}
