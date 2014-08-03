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

package com.untouchableapps.android.geodiscoverer;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.os.Environment;
import android.os.IBinder;
import android.os.Message;
import android.widget.Toast;

public class GDService extends Service {
  
  // Managers
  LocationManager locationManager;
  NotificationManager notificationManager;
  
  // Flags
  boolean locationWatchStarted = false;
  boolean serviceInForeground = false;

  /** Reference to the core object */
  GDCore coreObject = null;
  
  // The notification that is displayed in the status bar
  PendingIntent pendingIntent = null;
  Notification notification = null;

  // Variables for monitoring the state of the external storage
  BroadcastReceiver externalStorageReceiver = null;
  boolean externalStorageAvailable = false;
  boolean externalStorageWritable = false;  
      
  // Variables for handling the metwatch
  BroadcastReceiver metaWatchAppReceiver = null;
  boolean metaWatchAppActive = false;
  
  /** Called when the external storage state changes */
  synchronized void handleExternalStorageState() {

    // If the external storage is not available, inform the user that this will not work
    if ((!externalStorageAvailable)||(!externalStorageWritable)) {
      Toast.makeText(this, String.format(getString(R.string.no_external_storage)), Toast.LENGTH_LONG).show();
      if (coreObject.messageHandler!=null) {
        Message m=Message.obtain(coreObject.messageHandler);
        m.what = GDCore.HOME_DIR_NOT_AVAILABLE;
        coreObject.messageHandler.sendMessage(m);
      }
    }
    
    // If the external storage is available, start the native core
    if ((externalStorageAvailable)&&(externalStorageWritable)) {
      Message m=Message.obtain(coreObject.messageHandler);
      m.what = GDCore.HOME_DIR_AVAILABLE;
      coreObject.messageHandler.sendMessage(m);
    }

  }
  
  /** Sets the current state of the external media */
  void updateExternalStorageState() {
    String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state)) {
      externalStorageAvailable = externalStorageWritable = true;
    } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
      externalStorageAvailable = true;
      externalStorageWritable = false;
    } else {
      externalStorageAvailable = externalStorageWritable = false;
    }
    handleExternalStorageState();
  }
    
  /** Called when the service is created the first time */
  @Override
  public void onCreate() {
   
    super.onCreate();
    
    // Get the core object
    coreObject=GDApplication.coreObject;
    
    // Get the location manager
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    
    // Get the notification manager
    notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);    

    // Start watching the external storage state
    externalStorageReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "storage: " + intent.getData());
        updateExternalStorageState();
      }
    };
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    registerReceiver(externalStorageReceiver, filter);
    updateExternalStorageState();
    
    // Prepare the notification
    Intent notificationIntent = new Intent(this, ViewMap.class);
    pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);    
    
    // Register metawatch broadcast receiver
    metaWatchAppReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        if (metaWatchAppActive) {
          if (intent.getAction()=="org.metawatch.manager.BUTTON_PRESS") {
            MetaWatchApp.update(null, true);
          }
          if (intent.getAction()=="org.metawatch.manager.APPLICATION_DISCOVERY") {
            MetaWatchApp.announce(coreObject.application);
            MetaWatchApp.start();
          }
        }
      }
    };
    filter = new IntentFilter();
    filter.addAction("org.metawatch.manager.BUTTON_PRESS");
    filter.addAction("org.metawatch.manager.APPLICATION_DISCOVERY");
    registerReceiver(metaWatchAppReceiver, filter);
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

    // Ignore empty intents
    if ((intent==null)||(intent.getAction()==null))
      return;
    
    // Handle activity start / stop actions
    if (intent.getAction().equals("activityResumed")) {

      // If the external storage is available, enable the native core
      if ((externalStorageAvailable)&&(externalStorageWritable)) {
        Message m=Message.obtain(coreObject.messageHandler);
        m.what = GDCore.HOME_DIR_AVAILABLE;
        coreObject.messageHandler.sendMessage(m);
      }

      // Start watching the location
      if (!locationWatchStarted) {
        locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, coreObject);
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, coreObject);
        locationWatchStarted = true;
      }
            
      // Set the service to foreground
      if (!serviceInForeground) {
        notification = new Notification(R.drawable.status, getText(R.string.notification_service_in_foreground_message), System.currentTimeMillis());
        notification.setLatestEventInfo(this, getText(R.string.notification_title), getText(R.string.notification_service_in_foreground_message), pendingIntent);
        startForeground(R.string.notification_title, notification);
        serviceInForeground=true;
      }
    }
    if (intent.getAction().equals("coreInitialized")) {
      metaWatchAppActive = coreObject.configStoreGetStringValue("MetaWatch", "activateMetaWatchApp").equals("1");
    }
    if (intent.getAction().equals("activityPaused")) {
      
      // Stop watching location if track recording is disabled
      boolean recordingPosition = true;
      String state=coreObject.executeCoreCommand("getRecordTrack()");
      if (state.equals("false")||state.equals(""))
          recordingPosition = false;
      if ((!metaWatchAppActive)&&(!recordingPosition)) {
        locationManager.removeUpdates(coreObject);
        locationWatchStarted = false;
        if (serviceInForeground) {
          stopForeground(true);
          serviceInForeground=false;
        }
        stopSelf();
      }

    }
    
  }
  
  /** Called when the service is stopped */
  @Override
  public void onDestroy() {
    
    // Hide the notification
    stopForeground(true);
    //notificationManager.cancel(R.string.notification_title);

    // Stop watching for external storage
    unregisterReceiver(externalStorageReceiver);
    externalStorageReceiver=null;

    // Stop watching location
    locationManager.removeUpdates(coreObject);
    
    // Stop watch meta watch updates
    unregisterReceiver(metaWatchAppReceiver);
    metaWatchAppReceiver=null;
    
  }
  
  
  
}
