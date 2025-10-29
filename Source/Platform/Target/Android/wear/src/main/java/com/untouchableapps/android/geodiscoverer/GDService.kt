//============================================================================
// Name        : Recorder.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2025 Matthias Gruenewald
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
package com.untouchableapps.android.geodiscoverer

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.os.Message
import androidx.core.app.NotificationCompat
import androidx.wear.ongoing.OngoingActivity
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.ui.ViewMap

class GDService : Service() {
  // Managers
  var notificationManager: NotificationManager? = null

  // Flags
  var serviceInForeground: Boolean = false

  // Reference to the core object
  var coreObject: GDCore? = null

  // The notification that is displayed in the status bar
  var pendingIntent: PendingIntent? = null
  var notification: Notification? = null

  // Actions for notifications
  var exitAction: NotificationCompat.Action? = null

  /** Called when the service is created the first time  */
  override fun onCreate() {
    super.onCreate()

    // Get the core object
    coreObject = GDApplication.coreObject
    if (coreObject == null) {
      return
    }

    // Get the notification manager
    notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager

    // Create the notification channel
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.O) {
      val channel = NotificationChannel(
        "status",
        getString(R.string.notification_channel_status_name),
        NotificationManager.IMPORTANCE_LOW
      )
      channel.setDescription(getString(R.string.notification_channel_status_description))
      notificationManager!!.createNotificationChannel(channel)
    }

    // Make this activity an ongoing activity
    val pendingIntent =
      PendingIntent.getActivity(
        this,
        0,
        Intent(this, ViewMap::class.java).apply {
          flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
        },
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
      )
    val notificationBuilder = NotificationCompat.Builder(this, "status")
      .setContentTitle(getText(R.string.notification_title))
      .setContentText(getText(R.string.notification_service_in_foreground_message))
      .setSmallIcon(R.drawable.notification_running)
      .setCategory(NotificationCompat.CATEGORY_WORKOUT)
      .setContentIntent(pendingIntent)
      .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
      .setOngoing(true) // Important!
    val ongoingActivity =
      OngoingActivity.Builder(applicationContext, 0, notificationBuilder)
        .setAnimatedIcon(R.drawable.notification_running)
        .setStaticIcon(R.drawable.notification_running)
        .setTouchIntent(pendingIntent)
        .build()
    ongoingActivity.apply(applicationContext)
    startForeground(GDApplication.NOTIFICATION_STATUS_ID, notificationBuilder.build())


    /* Prepare the notification
    Intent notificationIntent = new Intent(this, ViewMap.class);
    pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
    Intent exitIntent = new Intent(this, GDService.class);
    exitIntent.setAction("exitApp");
    PendingIntent pendingExitIntent = PendingIntent.getService(this, 0, exitIntent, 0);
    exitAction = new NotificationCompat.Action.Builder(
        R.drawable.exit, "Exit", pendingExitIntent
    ).build();
    GDApplication.addMessage(
        GDApplication.DEBUG_MSG,
        "GDApp",
        "notification created"
    );*/
  }

  /** No binding supported  */
  override fun onBind(intent: Intent?): IBinder? {
    // We don't provide binding, so return null
    return null
  }

  /** Create the default notification  */
  private fun updateNotification(): Notification? {
    /* Handle download status updates
          NotificationCompat.Builder builder;
          if (tilesLeft==0) {
            builder = new NotificationCompat.Builder(this,"status")
                .setContentTitle(getText(R.string.notification_title))
                .setContentText(getText(R.string.notification_service_in_foreground_message))
                .setSmallIcon(R.drawable.notification_running)
                .setContentIntent(pendingIntent)
                //.setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
                .setOngoing(true)
                .addAction(exitAction);
          } else {
            builder = new NotificationCompat.Builder(this, "status")
                .setContentTitle(getText(R.string.notification_title))
                .setContentText(getString(R.string.notification_map_download_ongoing_message, tilesLeft, timeLeft))
                .setSmallIcon(R.drawable.notification_downloading)
                .setContentIntent(pendingIntent)
                //.setLargeIcon(BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher))
                .setOngoing(true)
                .setProgress(tilesTotal, tilesDone, false)
                .addAction(exitAction)
                .addAction(stopDownloadAction);
          }
          if ((heartRateService!=null)&&(heartRateService.heartRateUnavailableSound))
            builder.addAction(stopHeartRateUnavailableSoundAction);
          notification=builder.build();
          return notification;*/
    return null
  }

  /** Called when the service is started  */
  override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
    // Empty intents indicate a restart

    if (intent == null) {
      GDApplication.addMessage(
        GDApplication.DEBUG_MSG,
        "GDApp",
        "service has been restarted due to low memory situation"
      )
      return START_STICKY
    } else {
      if (intent.getAction() == null) return START_STICKY
    }

    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",intent.getAction());

    /* Handle activity start / stop actions
    if (intent.getAction().equals("activityResumed")) {
      activityRunning=true;
      activityStarted=true;
      startActivity=false;
      handleActivityStatus(false, false);
    }
    if (intent.getAction().equals("activityPaused")) {
      activityRunning=false;
      handleActivityStatus(false, false);
      if (coreObject!=null)
        coreObject.executeCoreCommand("maintenance");
    }*/

    // Handle start request
    if (intent.getAction() == "activityResumed") {
      if (coreObject != null) {
        val m = Message.obtain(coreObject!!.messageHandler)
        m.what = GDCore.START_CORE
        coreObject!!.messageHandler.sendMessage(m)
      }
    }

    // Handle start request
    if (intent.getAction() == "activityPaussed") {
    }

      // Handle exit request
    if (intent.getAction() == "exit") {
      if (coreObject != null) {
        val m = Message.obtain(coreObject!!.messageHandler)
        m.what = GDCore.STOP_CORE
        coreObject!!.messageHandler.sendMessage(m)
      }
      stopForeground(STOP_FOREGROUND_REMOVE)
      stopSelf()
    }

    return START_STICKY
  }

  /** Called when the service is stopped  */
  override fun onDestroy() {
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "service is beeing destroyed")

    // Hide the notification
    stopForeground(STOP_FOREGROUND_REMOVE)
    //notificationManager.cancel(R.string.notification_title);
  }
}
