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

package com.untouchableapps.android.geodiscoverer.logic;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.BitmapFactory;
import android.location.Address;
import android.location.Geocoder;
import android.location.LocationManager;
import android.os.AsyncTask;
import android.os.Build;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Settings;
import androidx.core.app.NotificationCompat;
import android.text.TextUtils;
import android.widget.Toast;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.R;
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.logic.ble.GDEBikeService;
import com.untouchableapps.android.geodiscoverer.logic.ble.GDHeartRateService;
import com.untouchableapps.android.geodiscoverer.logic.server.BRouterServer;
import com.untouchableapps.android.geodiscoverer.logic.server.MapTileServer;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class GDService extends Service {
  
  // Managers
  LocationManager locationManager;
  NotificationManager notificationManager;
  PowerManager powerManager;

  // Receivers
  BroadcastReceiver userPresentReceiver = null;

  // Flags
  boolean locationWatchStarted = false;
  boolean serviceInForeground = false;
  boolean activityRunning = false;
  boolean activityStarted = false;
  boolean startActivity = false;

  /** Reference to the core object */
  GDCore coreObject = null;

  /** Reference to the tile server */
  MapTileServer mapTileServer = null;

  /** Reference to the brouter server */
  BRouterServer brouterServer = null;

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

  /** Finds the geographic position for the given address */
  private class LocationFromAddressTask extends AsyncTask<Void, Void, Void> {

    String name;
    String address;
    String group;
    boolean locationFound=false;

    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      Geocoder geocoder = new Geocoder(GDService.this);
      try {
        List<Address> addresses = geocoder.getFromLocationName(address, 1);
        if (addresses.size()>0) {
          Address a = addresses.get(0);
          coreObject.scheduleCoreCommand("addAddressPoint",
              name, address,
              String.valueOf(a.getLongitude()), String.valueOf(a.getLatitude()),
              group);
          locationFound=true;
        } else {
          locationFound=false;
        }
      }
      catch(IOException e) {
        GDApplication.addMessage(GDApplication.WARNING_MSG, "GDApp", "Geocoding not successful: " + e.getMessage());
      }
      return null;
    }

    protected void onPostExecute(Void result) {
      if (!locationFound) {
        Toast.makeText(GDService.this,getString(R.string.address_point_not_found,name),Toast.LENGTH_LONG).show();
      } else {
        Toast.makeText(GDService.this,getString(R.string.address_point_added,name),Toast.LENGTH_LONG).show();
      }
    }
  }

  /** Checks if accessibility service is enabled */
  public boolean isAccessibilityEnabled() {
    int accessibilityEnabled = 0;
    final String ACCESSIBILITY_SERVICE_NAME = "com.untouchableapps.android.geodiscoverer/com.untouchableapps.android.geodiscoverer.logic.GDAccessibilityService";
    try {
      accessibilityEnabled = Settings.Secure.getInt(this.getContentResolver(),android.provider.Settings.Secure.ACCESSIBILITY_ENABLED);
    } catch (Settings.SettingNotFoundException e) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Error finding accessibility service setting: " + e.getMessage());
    }
    TextUtils.SimpleStringSplitter stringColonSplitter = new TextUtils.SimpleStringSplitter(':');
    if (accessibilityEnabled==1) {
      String settingValue = Settings.Secure.getString(getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp", "Setting: " + settingValue);
      if (settingValue != null) {
        TextUtils.SimpleStringSplitter splitter = stringColonSplitter;
        splitter.setString(settingValue);
        while (splitter.hasNext()) {
          String accessabilityService = splitter.next();
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Setting: " + accessabilityService);
          if (accessabilityService.equalsIgnoreCase(ACCESSIBILITY_SERVICE_NAME)){
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","We've found the correct setting - accessibility is switched on!");
            return true;
          }
        }
      }
    }
    return false;
  }

  /** Called when the service is created the first time */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  @Override
  public void onCreate() {
   
    super.onCreate();
    
    // Get the core object
    coreObject=GDApplication.coreObject;
    if (coreObject==null) {
      return;
    }

    // Get the location manager
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    
    // Get the notification manager
    notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

    // Get power manager
    powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);

    // Register for user present events
    userPresentReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        launchActivityIfNeeded();
      }
    };
    IntentFilter i=new IntentFilter(Intent.ACTION_USER_PRESENT);
    registerReceiver(userPresentReceiver,i);

    // Create the notification channel
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.O) {
      NotificationChannel channel = new NotificationChannel("accessibilityService",
          getString(R.string.notification_channel_accessibility_service_name),
          NotificationManager.IMPORTANCE_HIGH);
      channel.setDescription(getString(R.string.notification_channel_accessibility_service_description));
      notificationManager.createNotificationChannel(channel);
      channel = new NotificationChannel("status",
          getString(R.string.notification_channel_status_name),
          NotificationManager.IMPORTANCE_DEFAULT);
      channel.setDescription(getString(R.string.notification_channel_status_description));
      notificationManager.createNotificationChannel(channel);
    }

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

    // Check if accessibility service is enabled
    if (!isAccessibilityEnabled()) {
      notificationIntent = new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS);
      PendingIntent pi = PendingIntent.getActivity(this, 0, notificationIntent, 0);
      NotificationCompat.Builder builder = new NotificationCompat.Builder(this, "accessibilityService")
          .setContentTitle(getText(R.string.notification_accessibility_service_not_enabled_title))
          .setContentText(getString(R.string.notification_accessibility_service_not_enabled_text))
          .setContentIntent(pi)
          .setSmallIcon(R.drawable.notification_running)
          .setDefaults(Notification.DEFAULT_ALL)
          .setAutoCancel(true)
          .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
          .setPriority(NotificationCompat.PRIORITY_HIGH);
      notificationManager.notify(GDApplication.NOTIFICATION_ACCESSIBILITY_SERVICE_NOT_ENABLED_ID, builder.build());
    }
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
      builder = new NotificationCompat.Builder(this,"status")
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
      builder = new NotificationCompat.Builder(this, "status")
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

  /** Starts the view map activity if needed */
  void launchActivityIfNeeded() {
    if ((!activityRunning)&&(startActivity)) {
      Intent startViewMapIntent = new Intent(GDService.this, ViewMap.class);
      startViewMapIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      startActivity(startViewMapIntent);
    }
  }

  /** Updates the state depending on the activity status */
  private void handleActivityStatus(boolean forceStart) {

    // Now act depending if activity is running or not
    if ((activityRunning)||(forceStart)) {

      // Set the service to foreground
      if (!serviceInForeground) {
        notification = updateNotification();
        startForeground(GDApplication.NOTIFICATION_STATUS_ID, notification);
        serviceInForeground = true;
      }

      // Start watching the location
      if (!locationWatchStarted) {
        try {
          locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, coreObject);
        } catch (Exception e) {
        }
        ;
        try {
          locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, coreObject);
        } catch (Exception e) {
        }
        ;
        locationWatchStarted = true;
      }

      // Start the core if it is not already running
      if (coreObject.coreStopped) {
        Message m=Message.obtain(coreObject.messageHandler);
        m.what = GDCore.START_CORE;
        coreObject.messageHandler.sendMessage(m);
      }

      // Indicate that the activity must be started if the user is present
      if ((forceStart)&&(!activityStarted)) {
        startActivity = true;
        if (powerManager.isScreenOn()) {
          launchActivityIfNeeded();
        }
      }

    } else {

      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",coreObject.coreLateInitComplete ? "core is initialized" : "core is not initialized");

      // Don't act if GDCore is not initialized
      if (!coreObject.coreLateInitComplete)
        return;

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

  }

  /** Called when the service is started */
  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {

    // Is this a service restart?
    if ((intent!=null)&&(intent.getAction().equals("scheduledRestart"))) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "scheduled restart happened");
      intent=null;
    }

    // Empty intents indicate a restart
    if (intent==null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "service has been restarted due to low memory situation");

      // Start the core if the service is re-created
      handleActivityStatus(true);
      return START_STICKY;

    } else {
      if (intent.getAction()==null)
        return START_STICKY;
    }
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",intent.getAction());

    // Handle activity start / stop actions
    if (intent.getAction().equals("activityResumed")) {
      activityRunning=true;
      activityStarted=true;
      startActivity=false;
      handleActivityStatus(false);
    }
    if (intent.getAction().equals("activityPaused")) {
      activityRunning=false;
      handleActivityStatus(false);
      if (coreObject!=null)
        coreObject.executeCoreCommand("maintenance");
    }

    // Handle early init of the core
    if (intent.getAction().equals("earlyInitComplete")) {

      // Start the brouter server
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","starting brouter server");
      System.err.println("Test");
      if (brouterServer!=null) {
        brouterServer.stop();
      } else {
        brouterServer = new BRouterServer(coreObject);
      }
      brouterServer.start();

      // Start the map tile server
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","starting map tile server");
      if (mapTileServer!=null) {
        mapTileServer.stop();
      } else {
        mapTileServer = new MapTileServer((GDApplication)getApplication(),coreObject);
      }
      mapTileServer.start();
    }

    // Handle initialization of core
    if (intent.getAction().equals("coreInitialized")) {

      // Stop all bluetooth services
      if (heartRateService != null) {
        heartRateService.deinit();
      }
      if (eBikeService != null) {
        eBikeService.deinit();
      }

      // Start all bluetooth services
      if (GDHeartRateService.isSupported(this)) {
        heartRateService = new GDHeartRateService(this, coreObject);
      }

      // Start the ebike monitor (if supported)
      if (GDEBikeService.isSupported(this)) {
        eBikeService = new GDEBikeService(this, coreObject);
      }
    }

    // Handle late initialization of core
    if (intent.getAction().equals("lateInitComplete")) {

      // Update depending on the activity status
      handleActivityStatus(false);

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
      notificationManager.notify(GDApplication.NOTIFICATION_STATUS_ID, notification);
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",tilesLeft + " tiles left for downloading");
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
        notificationManager.notify(GDApplication.NOTIFICATION_STATUS_ID, notification);
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

      // Start the core (if it's not already)
      handleActivityStatus(true);

      if (coreObject.coreInitialized) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","updateFLStats intent received!");
        coreObject.configStoreSetStringValue("Forumslader", "connected", "1");
        coreObject.configStoreSetStringValue("Forumslader", "batteryLevel", String.valueOf(intent.getStringExtra("batteryLevel")));
        coreObject.configStoreSetStringValue("Forumslader", "powerDrawLevel", String.valueOf(intent.getStringExtra("powerDrawLevel")));
        coreObject.executeCoreCommand("dataChanged");
      }
    }

    // Handle new address point
    if (intent.getAction().equals("addAddressPoint")) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","adding address point " + intent.getStringExtra("name"));
      LocationFromAddressTask task = new LocationFromAddressTask();
      task.name=intent.getStringExtra("name");
      task.address=intent.getStringExtra("address");
      task.group=intent.getStringExtra("group");
      task.execute();
    }

    return START_STICKY;
  }
  
  /** Called when the service is stopped */
  @Override
  public void onDestroy() {

    // Stop the user present receiver
    if (userPresentReceiver!=null) {
      unregisterReceiver(userPresentReceiver);
    }

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

    // Stop the brouter
    if (brouterServer!=null) {
      brouterServer.stop();
    }

    // Stop the mapsforge server
    if (mapTileServer !=null) {
      mapTileServer.stop();
    }
  }
  
  
  
}