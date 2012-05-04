//============================================================================
// Name        : Recorder.java
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

package com.perfectapp.android.geodiscoverer;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

public class GDService extends Service {

  // For backward compatibility
  private static final Class[] mStartForegroundSignature = new Class[] {
    int.class, Notification.class};
  private static final Class[] mStopForegroundSignature = new Class[] {
      boolean.class};
  private NotificationManager mNM;
  private Method mStartForeground;
  private Method mStopForeground;
  private Object[] mStartForegroundArgs = new Object[2];
  private Object[] mStopForegroundArgs = new Object[1];
  
  // Managers
  LocationManager locationManager;
  
  // Flags
  boolean locationWatchStarted = false;

  /** Reference to the core object */
  GDCore coreObject = null;
  
  /** The notification that is displayed in the status bar */
  Notification notification = null;  

  // Variables for monitoring the state of the external storage
  BroadcastReceiver externalStorageReceiver;
  boolean externalStorageAvailable;
  boolean externalStorageWriteable;
  
  /** Called when the external storage state changes */
  synchronized void handleExternalStorageState(boolean externalStorageAvailable, boolean externalStorageWritable) {

    // If the external storage is not available, inform the user that this will not work
    if ((!externalStorageAvailable)||(!externalStorageWritable)) {
      Toast.makeText(this, String.format(getString(R.string.no_external_storage)), Toast.LENGTH_LONG).show();
      coreObject.stop();
    }
    
    // If the external storage is available, start the native core
    if ((externalStorageAvailable)&&(externalStorageWritable)) {
      coreObject.start();
    }

  }
  
  /** Sets the current state of the external media */
  void updateExternalStorageState() {
    String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state)) {
      externalStorageAvailable = externalStorageWriteable = true;
    } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
      externalStorageAvailable = true;
      externalStorageWriteable = false;
    } else {
      externalStorageAvailable = externalStorageWriteable = false;
    }
    handleExternalStorageState(externalStorageAvailable,externalStorageWriteable);
  }
    
  /** Called when the service is created the first time */
  @Override
  public void onCreate() {
   
    super.onCreate();
    
    // Find out on which platform the service is running
    mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
    try {
      mStartForeground = getClass().getMethod("startForeground",
              mStartForegroundSignature);
      mStopForeground = getClass().getMethod("stopForeground",
              mStopForegroundSignature);
    } catch (NoSuchMethodException e) {
      // Running on an older platform.
      mStartForeground = mStopForeground = null;
    }  

    // Get the core object
    GDApplication app=(GDApplication)getApplication();
    coreObject=app.coreObject;
    
    // Start watching the external storage state
    externalStorageReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        Log.i("GDApp", "Storage: " + intent.getData());
        updateExternalStorageState();
      }
    };
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    registerReceiver(externalStorageReceiver, filter);
    updateExternalStorageState();
  }
  
  /**
   * This is a wrapper around the new startForeground method, using the older
   * APIs if it is not available.
   */
  void startForegroundCompat(int id, Notification notification) {
    // If we have the new startForeground API, then use it.
    if (mStartForeground != null) {
      mStartForegroundArgs[0] = Integer.valueOf(id);
      mStartForegroundArgs[1] = notification;
      try {
        mStartForeground.invoke(this, mStartForegroundArgs);
      } catch (InvocationTargetException e) {
        // Should not happen.
        Log.w("GDApp", "Unable to invoke startForeground", e);
      } catch (IllegalAccessException e) {
        // Should not happen.
        Log.w("GDApp", "Unable to invoke startForeground", e);
      }
      return;
    }
    
    // Fall back on the old API.
    setForeground(true);
    mNM.notify(id, notification);
  }
  
  /**
   * This is a wrapper around the new stopForeground method, using the older
   * APIs if it is not available.
   */
  void stopForegroundCompat(int id) {
    // If we have the new stopForeground API, then use it.
    if (mStopForeground != null) {
      mStopForegroundArgs[0] = Boolean.TRUE;
      try {
        mStopForeground.invoke(this, mStopForegroundArgs);
      } catch (InvocationTargetException e) {
        // Should not happen.
        Log.w("GDApp", "Unable to invoke stopForeground", e);
      } catch (IllegalAccessException e) {
        // Should not happen.
        Log.w("GDAPp", "Unable to invoke stopForeground", e);
      }
      return;
    }
      
    // Fall back on the old API.  Note to cancel BEFORE changing the
    // foreground state, since we could be killed at that point.
    mNM.cancel(id);
    setForeground(false);
  }  

  /** No binding supported */
  @Override
  public IBinder onBind(Intent intent) {
      // We don't provide binding, so return null
      return null;
  }
  
  /** Called when the service is started */
  @Override
  public void onStart(Intent intent, int startId) {    
    
    // Inform the user that the service has started
    if (notification == null) {
      notification = new Notification(R.drawable.icon, getText(R.string.notification_message), System.currentTimeMillis());
      Intent notificationIntent = new Intent(this, ViewMap.class);
      PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
      notification.setLatestEventInfo(this, getText(R.string.notification_title), getText(R.string.notification_message), pendingIntent);
      startForegroundCompat(R.string.notification_message, notification);
    }
    
    // Handle activity start / stop actions
    if (intent.getAction().equals("activityResumed")) {

      // Start watching the location
      if (!locationWatchStarted) {
        locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
        locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, coreObject);
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, coreObject);
        locationWatchStarted = true;
      }
      
    }
    if (intent.getAction().equals("activityPaused")) {
      
      // Stop watching location if track recording is disabled
      String state=coreObject.executeCoreCommand("getRecordTrack()");
      if (state.equals("false")) {
        locationManager.removeUpdates(coreObject);
        locationWatchStarted = false;
        stopSelf();
      }

    }
    
  }
  
  /** Called when the service is stopped */
  @Override
  public void onDestroy() {
    
    // Hide the notification
    stopForegroundCompat(R.string.notification_message);

    // Stop watching for external storage
    unregisterReceiver(externalStorageReceiver);
    externalStorageReceiver=null;

    // Stop watching location
    locationManager.removeUpdates(coreObject);
  }
  
}
