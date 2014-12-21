//============================================================================
// Name        : GDDeviceAdminReceiver.java
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

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

/* Handles messages from the device administrator service */
public class GDDeviceAdminReceiver extends DeviceAdminReceiver {

  void showToast(Context context, String msg) {
    Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onEnabled(Context context, Intent intent) {
      //showToast(context, "enabled");
  }

  @Override
  public void onDisabled(Context context, Intent intent) {
      //showToast(context, "disabled");
  }

}
