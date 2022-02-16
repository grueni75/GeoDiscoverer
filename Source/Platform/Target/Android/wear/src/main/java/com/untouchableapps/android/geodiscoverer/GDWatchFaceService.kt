//============================================================================
// Name        : GDWatchFaceService
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2022 Matthias Gruenewald
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

import android.content.Intent
import android.view.SurfaceHolder
import androidx.wear.watchface.ComplicationSlotsManager
import androidx.wear.watchface.WatchFace
import androidx.wear.watchface.WatchFaceType
import androidx.wear.watchface.WatchState
import androidx.wear.watchface.style.CurrentUserStyleRepository
import java.lang.ref.WeakReference
import java.util.*
import com.untouchableapps.android.geodiscoverer.core.GDCore
import android.annotation.SuppressLint
import android.app.AlarmManager
import android.hardware.SensorManager
import android.os.*
import android.view.WindowManager
import android.os.Build

import android.content.pm.ConfigurationInfo

import android.app.ActivityManager

import android.os.PowerManager

import android.app.NotificationManager

import android.os.Vibrator

import android.view.LayoutInflater





// Minimum distance between two toasts in milliseconds  */
const val TOAST_DISTANCE = 5000

class GDWatchFaceService : androidx.wear.watchface.WatchFaceService() {

  // Managers
  var powerManager: PowerManager? = null
  var vibrator: Vibrator? = null

  // Reference to the core object
  var coreObject: GDCore? = null

  // The last renderer created
  var lastRenderer: WatchFaceRenderer? = null

  // Indicates if the dialog is open
  var dialogVisible = false

  // Time the last toast was shown
  var lastToastTimestamp: Long = 0

  // Wake lock for the core
  var wakeLockCore: PowerManager.WakeLock? = null

  // Short vibration to give feedback to user
  fun vibrate() {
    vibrator?.vibrate(VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE))
  }

  // Shows a dialog
  @Synchronized
  fun dialog(kind: Int, message: String) {
    if ((kind == Dialog.Types.WARNING)||(kind == Dialog.Types.INFO)) {
      val diff: Long = SystemClock.uptimeMillis() - lastToastTimestamp
      if (diff <= TOAST_DISTANCE) {
        GDApplication.addMessage(
          GDApplication.DEBUG_MSG, "GDApp",
          "skipping dialog request <$message> because toast is still visible"
        )
        return
      }
      GDApplication.showMessageBar(
        applicationContext,
        message,
        GDApplication.MESSAGE_BAR_DURATION_LONG
      )
      lastToastTimestamp = SystemClock.uptimeMillis()
    } else {
      val intent = Intent(this, Dialog::class.java)
      intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_NO_ANIMATION
      intent.putExtra(Dialog.Extras.TEXT, message)
      intent.putExtra(Dialog.Extras.KIND, kind)
      startActivity(intent)
    }
  }

  /** Shows a fatal dialog and quits the applications  */
  fun fatalDialog(message: String) {
    dialog(Dialog.Types.FATAL, message)
  }

  /** Shows an error dialog without quitting the application  */
  fun errorDialog(message: String) {
    dialog(Dialog.Types.ERROR, message)
  }

  /** Shows a warning dialog without quitting the application  */
  fun warningDialog(message: String) {
    dialog(Dialog.Types.WARNING, message)
  }

  /** Shows an info dialog without quitting the application  */
  fun infoDialog(message: String) {
    dialog(Dialog.Types.INFO, message)
  }

  // Sets the wake lock controlled by the core
  @SuppressLint("Wakelock")
  fun updateWakeLock() {
    if (wakeLockCore==null)
      return
    val state = coreObject?.executeCoreCommand("getWakeLock")
    if (state == "true") {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock enabled")
      if (!wakeLockCore!!.isHeld) wakeLockCore?.acquire()
    } else {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock disabled")
      if (wakeLockCore!!.isHeld) wakeLockCore?.release()
    }
  }

  // Communication with the native core
  class CoreMessageHandler(GDWatchFace: GDWatchFaceService) : Handler(Looper.getMainLooper()) {
    private val weakGDWatchFace: WeakReference<GDWatchFaceService> = WeakReference(GDWatchFace)

    /** Called when the core has a message  */
    override fun handleMessage(msg: Message) {

      // Abort if the object is not available anymore
      val watchFaceService: GDWatchFaceService = weakGDWatchFace.get() ?: return

      // Handle the message
      val b: Bundle = msg.data
      when (msg.what) {
        0 -> {

          // Extract the command
          val command = b.getString("command")
          val args_start = command!!.indexOf("(")
          val args_end = command.lastIndexOf(")")
          val commandFunction = command.substring(0, args_start)
          val t = command.substring(args_start + 1, args_end)
          val commandArgs: Vector<String> = Vector<String>()
          var stringStarted = false
          var startPos = 0
          var i = 0
          while (i < t.length) {
            if (t.substring(i, i + 1) == "\"") {
              stringStarted = !stringStarted
            }
            if (!stringStarted) {
              if (t.substring(i, i + 1) == "," || i == t.length - 1) {
                var arg: String
                arg =
                  if (i == t.length - 1) t.substring(startPos, i + 1) else t.substring(startPos, i)
                if (arg.startsWith("\"")) {
                  arg = arg.substring(1)
                }
                if (arg.endsWith("\"")) {
                  arg = arg.substring(0, arg.length - 1)
                }
                commandArgs.add(arg)
                startPos = i + 1
              }
            }
            i++
          }
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","command received: " + command);

          // Execute command
          var commandExecuted = false
          if (commandFunction == "fatalDialog") {
            watchFaceService.fatalDialog(commandArgs.get(0))
            commandExecuted = true
          }
          if (commandFunction == "errorDialog") {
            watchFaceService.errorDialog(commandArgs.get(0))
            commandExecuted = true
          }
          if (commandFunction == "warningDialog") {
            watchFaceService.warningDialog(commandArgs.get(0))
            commandExecuted = true
          }
          if (commandFunction == "infoDialog") {
            watchFaceService.infoDialog(commandArgs.get(0))
            commandExecuted = true
          }
          if (commandFunction == "createProgressDialog") {

            // Create a new dialog if it does not yet exist
            if (!watchFaceService.dialogVisible) {
              val intent = Intent(watchFaceService, Dialog::class.java)
              intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_NO_ANIMATION
              intent.putExtra(Dialog.Extras.TEXT, commandArgs.get(0))
              val max: Int = commandArgs.get(1).toInt()
              intent.putExtra(Dialog.Extras.MAX, max)
              watchFaceService.startActivity(intent)
              watchFaceService.dialogVisible = true
            } else {
              GDApplication.addMessage(
                GDApplication.DEBUG_MSG,
                "GDApp",
                "skipping progress dialog request <" + commandArgs.get(0)
                  .toString() + "> because progress dialog is already visible"
              )
            }
            commandExecuted = true
          }
          if (commandFunction == "updateProgressDialog") {
            if (watchFaceService.dialogVisible) {
              val intent = Intent(watchFaceService, Dialog::class.java)
              intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_NO_ANIMATION
              intent.putExtra(Dialog.Extras.PROGRESS, commandArgs.get(1).toInt())
              watchFaceService.startActivity(intent)
            }
            commandExecuted = true
          }
          if (commandFunction == "closeProgressDialog") {
            if (watchFaceService.dialogVisible) {
              val intent = Intent(watchFaceService, Dialog::class.java)
              intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_NO_ANIMATION
              intent.putExtra(Dialog.Extras.CLOSE, true)
              watchFaceService.startActivity(intent)
              watchFaceService.dialogVisible = false
            }
            commandExecuted = true
          }
          if (commandFunction == "coreInitialized") {
            commandExecuted = true
          }
          if (commandFunction == "earlyInitComplete") {
            // Nothing to do as of now
            commandExecuted = true
          }
          if (commandFunction == "lateInitComplete") {
            // Nothing to do as of now
            commandExecuted = true
          }
          if (commandFunction == "setSplashVisibility") {
            // Nothing to do as of now
            commandExecuted = true
          }
          if (commandFunction == "updateWakeLock") {
            watchFaceService.updateWakeLock()
            commandExecuted = true
          }
          if (commandFunction == "updateScreen") {
            if (watchFaceService.lastRenderer != null) {
              //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","invalidating screen");
              watchFaceService.lastRenderer?.forceRedraw()
            }
            commandExecuted = true
          }
          if (commandFunction == "restartActivity") {
            if (GDApplication.coreObject != null) {
              val m: Message = Message.obtain(GDApplication.coreObject.messageHandler)
              m.what = GDCore.START_CORE
              GDApplication.coreObject.messageHandler.sendMessage(m)
            }
            commandExecuted = true
          }
          if (commandFunction == "exitActivity") {
            commandExecuted = true
          }
          if (commandFunction == "deactivateSwipes") {
            watchFaceService.lastRenderer?.setTouchHandlerEnabled(false)
            watchFaceService.vibrate()
            commandExecuted = true
          }
          if (!commandExecuted) {
            GDApplication.addMessage(
              GDApplication.ERROR_MSG, "GDApp",
              "unknown command $command received"
            )
          }
        }
      }
    }
  }
  var coreMessageHandler = CoreMessageHandler(this)

  /** Gets the vibrator in legacy way  */
  @SuppressLint("deprecation")
  private fun getLegacyVibrator(): Vibrator? {
    return getSystemService(VIBRATOR_SERVICE) as Vibrator;
  }

  // Init everything
  override fun onCreate() {
    super.onCreate()

    // Get the core object
    coreObject = GDApplication.coreObject
    (application as GDApplication).setMessageHandler(coreMessageHandler)

    // Get the managers
    powerManager = getSystemService(POWER_SERVICE) as PowerManager
    vibrator = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      (getSystemService(VIBRATOR_MANAGER_SERVICE) as VibratorManager).defaultVibrator
    } else {
      getLegacyVibrator()
    }

    // Get a wake lock
    wakeLockCore = powerManager?.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "GDApp: Active core");
    if (wakeLockCore==null) {
      fatalDialog(getString(R.string.no_wake_lock))
      return
    }

    // Check for OpenGL ES 2.00
    val activityManager = getSystemService(ACTIVITY_SERVICE) as ActivityManager
    val configurationInfo = activityManager.deviceConfigurationInfo
    val supportsEs2 =
      configurationInfo.reqGlEsVersion >= 0x20000 || Build.FINGERPRINT.startsWith("generic")
    if (!supportsEs2) {
      fatalDialog(getString(R.string.opengles20_required))
      return
    }
  }

  // Returns a new watch face
  override suspend fun createWatchFace(
    surfaceHolder: SurfaceHolder,
    watchState: WatchState,
    complicationSlotsManager: ComplicationSlotsManager,
    currentUserStyleRepository: CurrentUserStyleRepository
  ): WatchFace {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","createWatchFace called")

    // Creates class that renders the watch face.
    lastRenderer = WatchFaceRenderer(
      context = applicationContext,
      coreObject = coreObject,
      surfaceHolder = surfaceHolder,
      watchState = watchState,
      currentUserStyleRepository = currentUserStyleRepository
    )

    // Creates the watch face.
    val watchFace = WatchFace(
      watchFaceType = WatchFaceType.DIGITAL,
      renderer = lastRenderer!!
    )
    watchFace.setTapListener(lastRenderer)
    return watchFace
  }

  // Called when the service is destroyed
  override fun onDestroy() {
    super.onDestroy()
    if ((wakeLockCore!=null)&&(wakeLockCore!!.isHeld))
      wakeLockCore?.release();
    (getApplication() as GDApplication).setMessageHandler(null)
    if (coreObject!=null) {
      if (!coreObject!!.coreStopped) {
        val m: Message = Message.obtain(coreObject!!.messageHandler)
        m.what = GDCore.STOP_CORE
        coreObject!!.messageHandler.sendMessage(m)
      }
    }
  }
}