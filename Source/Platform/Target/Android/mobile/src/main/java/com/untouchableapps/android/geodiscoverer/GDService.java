//============================================================================
// Name        : Recorder.java
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
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.location.LocationManager;
import android.os.Build;
import android.os.Environment;
import android.os.IBinder;
import android.os.Message;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.support.v7.app.NotificationCompat;
import android.widget.Toast;

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.io.File;

public class GDService extends Service {
  
  // Managers
  LocationManager locationManager;
  NotificationManager notificationManager;

  // Flags
  boolean locationWatchStarted = false;
  boolean serviceInForeground = false;
  boolean serviceRestarted = false;
  
  /** Reference to the core object */
  GDCore coreObject = null;
  
  // The notification that is displayed in the status bar
  PendingIntent pendingIntent = null;
  Notification notification = null;

  /** Called when the service is created the first time */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  @Override
  public void onCreate() {
   
    super.onCreate();
    
    // Get the core object
    coreObject=GDApplication.coreObject;

    // Get the location manager
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    
    // Get the notification manager
    notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);    

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
        .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
        .build();
  }

  /** Called when the service is started */
  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {

    // Ignore empty intents
    if (intent==null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "service has been restarted due to low memory situation");

      // Start the core if the service is re-created
      if (coreObject.coreStopped) {
        serviceRestarted=true;
        Message m = Message.obtain(coreObject.messageHandler);
        m.what = GDCore.START_CORE;
        coreObject.messageHandler.sendMessage(m);
      }

      return START_STICKY;

    } else {
      if (intent.getAction()==null)
        return START_STICKY;
    }
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDapp",intent.getAction());

    // Handle activity start / stop actions
    if ((serviceRestarted)||(intent.getAction().equals("activityResumed"))) {

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
    if ((serviceRestarted)||(intent.getAction().equals("activityPaused"))) {

      // Stop watching location if track recording is disabled
      boolean recordingPosition = true;
      String state=coreObject.executeCoreCommand("getRecordTrack()");
      if (state.equals("false")||state.equals(""))
          recordingPosition = false;
      boolean downloadingMaps = true;
      state=coreObject.executeCoreCommand("getMapDownloadActive()");
      if (state.equals("false")||state.equals(""))
        downloadingMaps = false;
      if ((!((GDApplication)getApplication()).cockpitEngineIsActive())&&(!recordingPosition)&&(!downloadingMaps)) {
        locationManager.removeUpdates(coreObject);
        locationWatchStarted = false;
        if (serviceInForeground) {
          stopForeground(true);
          serviceInForeground=false;
        }
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "stopping service");
        stopSelf();
      }

    }

    // Handle initialization of core
    if (intent.getAction().equals("lateInitComplete")) {

      // Service restart is complete
      serviceRestarted=false;

      // Replay the trace if it exists
      File replayLog = new File(coreObject.homePath + "/replay.log");
      if (replayLog.exists()) {
        new Thread() {
          public void run() {
            coreObject.executeCoreCommand("replayTrace(" + coreObject.homePath + "/replay.log" + ")");
          }
        }.start();
      }
    }

    // Handle download status updates
    if (intent.getAction().equals("mapDownloadStatusUpdated")) {
      if (intent.getIntExtra("tilesLeft",0)==0) {
        notification = createDefaultNotification();
      } else {
        int tilesDone = intent.getIntExtra("tilesDone", 0);
        int tilesTotal = intent.getIntExtra("tilesLeft", 0) + tilesDone;
        /*Drawable notificationDownloadingDrawble= ResourcesCompat.getDrawable(getResources(),
            R.drawable.notification_downloading,null);
        DrawableCompat.setTint(notificationDownloadingDrawble,Color.WHITE);*/
        notification = new NotificationCompat.Builder(this)
            .setContentTitle(getText(R.string.notification_title))
            .setContentText(getString(R.string.notification_map_download_ongoing_message,
                intent.getIntExtra("tilesLeft", 0), intent.getStringExtra("timeLeft")))
            .setSmallIcon(R.drawable.notification_downloading)
            .setContentIntent(pendingIntent)
            .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
            .setProgress(tilesTotal, tilesDone, false)
            .build();
      }
      notificationManager.notify(R.string.notification_title, notification);
    }

    return START_STICKY;
  }
  
  /** Called when the service is stopped */
  @Override
  public void onDestroy() {
    
    // Hide the notification
    stopForeground(true);
    //notificationManager.cancel(R.string.notification_title);

    // Stop watching location
    locationManager.removeUpdates(coreObject);
  }
  
  
  
}
