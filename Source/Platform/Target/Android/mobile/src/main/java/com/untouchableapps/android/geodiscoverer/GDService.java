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
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.location.LocationManager;
import android.os.Build;
import android.os.IBinder;
import android.os.Message;
import android.support.v4.app.NotificationCompat;

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.server.MapTileServer;

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

  /** Reference to the tile server */
  MapTileServer mapTileServer = null;

  // The notification that is displayed in the status bar
  PendingIntent pendingIntent = null;
  Notification notification = null;

  // Actions for notifications
  NotificationCompat.Action exitAction = null;
  NotificationCompat.Action stopDownloadAction = null;
  NotificationCompat.Action stopHeartRateUnavailableSoundAction = null;

  // Heart rate monitor
  GDHeartRateService heartRateService = null;

  // E-Bike monitor
  GDEBikeService eBikeService = null;

  // Download status
  int tilesLeft;
  int tilesDone;
  int tilesTotal;
  String timeLeft;

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
    Intent exitIntent = new Intent(this, GDService.class);
    exitIntent.setAction("exitApp");
    PendingIntent pendingExitIntent = PendingIntent.getService(this, 0, exitIntent, 0);
    exitAction = new NotificationCompat.Action.Builder(
        R.drawable.exit, "Exit", pendingExitIntent
    ).build();
    Intent stopDownloadIntent = new Intent(this, GDService.class);
    stopDownloadIntent.setAction("stopDownload");
    PendingIntent pendingStopDownloadIntent = PendingIntent.getService(this, 0, stopDownloadIntent, 0);
    stopDownloadAction = new NotificationCompat.Action.Builder(
        R.drawable.stop, "Stop Download", pendingStopDownloadIntent
    ).build();
    Intent stopHeartRateUnavailableSoundIntent = new Intent(this, GDService.class);
    stopHeartRateUnavailableSoundIntent.setAction("stopHeartRateUnavailableSound");
    PendingIntent pendingStopHeartRateUnavailableSoundIntent = PendingIntent.getService(this, 0, stopHeartRateUnavailableSoundIntent, 0);
    stopHeartRateUnavailableSoundAction = new NotificationCompat.Action.Builder(
        R.drawable.mute, "No Heartrate", pendingStopHeartRateUnavailableSoundIntent
    ).build();
  }
  
  /** No binding supported */
  @Override
  public IBinder onBind(Intent intent) {
      // We don't provide binding, so return null
      return null;
  }

  /** Create the default notification */
  private Notification updateNotification() {

    // Handle download status updates
    NotificationCompat.Builder builder;
    if (tilesLeft==0) {
      builder = new NotificationCompat.Builder(this)
          .setContentTitle(getText(R.string.notification_title))
          .setContentText(getText(R.string.notification_service_in_foreground_message))
          .setSmallIcon(R.drawable.notification_running)
          .setContentIntent(pendingIntent)
          .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
          .addAction(exitAction);
    } else {
      /*Drawable notificationDownloadingDrawble= ResourcesCompat.getDrawable(getResources(),
          R.drawable.notification_downloading,null);
      DrawableCompat.setTint(notificationDownloadingDrawble,Color.WHITE);*/
      builder = new NotificationCompat.Builder(this)
          .setContentTitle(getText(R.string.notification_title))
          .setContentText(getString(R.string.notification_map_download_ongoing_message, tilesLeft, timeLeft))
          .setSmallIcon(R.drawable.notification_downloading)
          .setContentIntent(pendingIntent)
          .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
          .setProgress(tilesTotal, tilesDone, false)
          .addAction(exitAction)
          .addAction(stopDownloadAction);
    }
    if ((heartRateService==null)||(heartRateService.heartRateUnavailableSound))
      builder.addAction(stopHeartRateUnavailableSoundAction);
    notification=builder.build();
    return notification;
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
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",intent.getAction());

    // Check if we need to restart listening to location updates
    boolean initService=false;
    if ((serviceRestarted)&&(intent.getAction().equals("lateInitComplete"))) {
      initService=true;
      serviceRestarted=false;
    }

    // Handle activity start / stop actions
    if ((initService)||(intent.getAction().equals("activityResumed"))) {

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
        notification=updateNotification();
        startForeground(R.string.notification_title, notification);
        serviceInForeground=true;
      }
    }
    if ((initService)||(intent.getAction().equals("activityPaused"))) {

      // Stop watching location if track recording is disabled
      boolean recordingPosition = true;
      String state=coreObject.executeCoreCommand("getRecordTrack");
      if (state.equals("false")||state.equals(""))
          recordingPosition = false;
      boolean downloadingMaps = true;
      state=coreObject.executeCoreCommand("getMapDownloadActive");
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

      // Stop all bluetooth services
      if (heartRateService != null) {
        heartRateService.deinit();
      }
      if (eBikeService != null) {
        eBikeService.deinit();
      }

      // Start all bluetooth services
      if (GDHeartRateService.isSupported(this)) {
        heartRateService = new GDHeartRateService(this,coreObject);
      }

      // Start the ebike monitor (if supported)
      if (GDEBikeService.isSupported(this)) {
        eBikeService = new GDEBikeService(this,coreObject);
      }


      // Replay the trace if it exists
      File replayLog = new File(coreObject.homePath + "/replay.log");
      if (replayLog.exists()) {
        new Thread() {
          public void run() {
            coreObject.executeCoreCommand("replayTrace",coreObject.homePath + "/replay.log");
          }
        }.start();
      }
    }

    // Handle download status updates
    if (intent.getAction().equals("mapDownloadStatusUpdated")) {
      tilesLeft=intent.getIntExtra("tilesLeft",0);
      tilesDone = intent.getIntExtra("tilesDone", 0);
      tilesTotal = intent.getIntExtra("tilesLeft", 0) + tilesDone;
      timeLeft = intent.getStringExtra("timeLeft");
      notification = updateNotification();
      notificationManager.notify(R.string.notification_title, notification);
    }

    // Handle exit request
    if (intent.getAction().equals("exitApp")) {
      if (coreObject!=null) {
        ViewMap viewMap = ((GDApplication)getApplication()).activity;
        if (viewMap!=null) {
          viewMap.setExitBusyText();
        }
        Message m=Message.obtain(coreObject.messageHandler);
        m.what = GDCore.STOP_CORE;
        coreObject.messageHandler.sendMessage(m);
      }
    }

    // Handle stop heart rate unavailable sound request
    if (intent.getAction().equals("stopHeartRateUnavailableSound")) {
      if (heartRateService!=null) {
        heartRateService.heartRateUnavailableSound=false;
        notification = updateNotification();
        notificationManager.notify(R.string.notification_title, notification);
      }
    }

    // Handle stop download request
    if (intent.getAction().equals("stopDownload")) {
      if (coreObject!=null) {
        coreObject.executeCoreCommand("stopDownload");
      }
    }

    // Handle start of the mapsforge server
    if (intent.getAction().equals("startMapTileServer")) {
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","starting map tile server");
      if (mapTileServer!=null) {
        mapTileServer.stop();
      } else {
        mapTileServer = new MapTileServer((GDApplication)getApplication(),coreObject);
      }
      mapTileServer.start();
    }

    // Handle updates from the tandem tracker
    if (intent.getAction().equals("updateFLStats")) {
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","updateFLStats intent received!");
      coreObject.configStoreSetStringValue("Forumslader","connected","1");
      coreObject.configStoreSetStringValue("Forumslader","batteryLevel",String.valueOf(intent.getIntExtra("batteryLevel",0)));
      coreObject.configStoreSetStringValue("Forumslader","powerDrawLevel",String.valueOf(intent.getIntExtra("powerDrawLevel",0)));
      coreObject.executeCoreCommand("dataChanged");
    }

    return START_STICKY;
  }
  
  /** Called when the service is stopped */
  @Override
  public void onDestroy() {

    // Stop the bluetooth services
    if (heartRateService!=null) {
      heartRateService.deinit();
      heartRateService=null;
    }
    if (eBikeService!=null) {
      eBikeService.deinit();
      eBikeService=null;
    }

    // Hide the notification
    stopForeground(true);
    //notificationManager.cancel(R.string.notification_title);

    // Stop watching location
    locationManager.removeUpdates(coreObject);

    // Stop the mapsforge server
    if (mapTileServer !=null) {
      mapTileServer.stop();
    }
  }
  
  
  
}
