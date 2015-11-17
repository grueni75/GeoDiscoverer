//============================================================================
// Name        : Recorder.java
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

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.BitmapFactory;
import android.location.LocationManager;
import android.os.Build;
import android.os.Environment;
import android.os.IBinder;
import android.os.Message;
import android.support.v7.app.NotificationCompat;
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
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  @Override
  public void onCreate() {
   
    super.onCreate();
    
    // Get the core object
    coreObject=GDApplication.coreObject;
    
    // Set the screen DPI
    coreObject.setDisplayMetrics(getResources().getDisplayMetrics());
    
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
  }
  
  /** No binding supported */
  @Override
  public IBinder onBind(Intent intent) {
      // We don't provide binding, so return null
      return null;
  }

  /** Create the default notification */
  private Notification createDefaultNotification() {
    return new NotificationCompat.Builder(this)
        .setContentTitle(getText(R.string.notification_title))
        .setContentText(getText(R.string.notification_service_in_foreground_message))
        .setSmallIcon(R.drawable.notification_running)
        .setContentIntent(pendingIntent)
        .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.ic_launcher))
        .build();
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
        try {
          locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, coreObject);
        } 
        catch (Exception e) {};
        try {
          locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, coreObject);
        }
        catch (Exception e) {};
        locationWatchStarted = true;
      }
            
      // Set the service to foreground
      if (!serviceInForeground) {
        notification=createDefaultNotification();
        startForeground(R.string.notification_title, notification);
        serviceInForeground=true;
      }
    }
    if (intent.getAction().equals("activityPaused")) {

      // Stop watching location if track recording is disabled
      boolean recordingPosition = true;
      String state=coreObject.executeCoreCommand("getRecordTrack()");
      if (state.equals("false")||state.equals(""))
          recordingPosition = false;
      boolean downloadingMaps = true;
      state=coreObject.executeCoreCommand("getMapDownloadActive()");
      if (state.equals("false")||state.equals(""))
        downloadingMaps = false;
      if ((!coreObject.cockpitEngineIsActive())&&(!recordingPosition)&&(!downloadingMaps)) {
        locationManager.removeUpdates(coreObject);
        locationWatchStarted = false;
        if (serviceInForeground) {
          stopForeground(true);
          serviceInForeground=false;
        }
        stopSelf();
      }

    }
    if (intent.getAction().equals("mapDownloadStatusUpdated")) {
      if (intent.getIntExtra("tilesLeft",0)==0) {
        notification = createDefaultNotification();
      } else {
        int tilesDone=intent.getIntExtra("tilesDone",0);
        int tilesTotal=intent.getIntExtra("tilesLeft",0)+tilesDone;
        notification = new NotificationCompat.Builder(this)
            .setContentTitle(getText(R.string.notification_title))
            .setContentText(getString(R.string.notification_map_download_ongoing_message,
                            intent.getIntExtra("tilesLeft",0),intent.getStringExtra("timeLeft")))
            .setSmallIcon(R.drawable.notification_downloading)
            .setContentIntent(pendingIntent)
            .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.ic_launcher))
            .setProgress(tilesTotal,tilesDone,false)
            .build();
      }
      notificationManager.notify(R.string.notification_title, notification);
    }

  }
  
  /** Called when the service is stopped */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
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
  }
  
  
  
}
