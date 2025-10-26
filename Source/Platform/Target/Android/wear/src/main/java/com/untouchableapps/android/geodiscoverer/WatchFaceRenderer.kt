//============================================================================
// Name        : WatchFaceRenderer
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

import android.Manifest
import android.R.attr.x
import android.R.attr.y
import android.annotation.SuppressLint
import android.content.Context
import android.content.Context.*
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import android.hardware.Sensor
import android.hardware.SensorManager
import android.opengl.GLES20
import android.os.*
import android.provider.Settings
import android.provider.Settings.SettingNotFoundException
import android.view.*
import androidx.wear.watchface.*
import androidx.wear.watchface.style.CurrentUserStyleRepository
import com.untouchableapps.android.geodiscoverer.core.GDCore
import kotlinx.coroutines.*
import java.time.ZonedDateTime
import java.util.*

/*

// Default for how long each frame is displayed at expected frame rate.
private const val FRAME_PERIOD_MS_DEFAULT: Long = 16L

// Types of dialogs
private const val FATAL_DIALOG = 0
private const val ERROR_DIALOG = 2
private const val WARNING_DIALOG = 1
private const val INFO_DIALOG = 3

@SuppressLint("ClickableViewAccessibility")
class WatchFaceRenderer(
  val context: Context,
  val coreObject: GDCore?,
  surfaceHolder: SurfaceHolder,
  val watchState: WatchState,
  currentUserStyleRepository: CurrentUserStyleRepository,
) : Renderer.GlesRenderer(
  surfaceHolder,
  currentUserStyleRepository,
  watchState,
  FRAME_PERIOD_MS_DEFAULT
), CoroutineScope by MainScope(), WatchFace.TapListener {

  // Managers
  val powerManager = context.getSystemService(POWER_SERVICE) as PowerManager
  val windowManager = context.getSystemService(WINDOW_SERVICE) as WindowManager
  val sensorManager = context.getSystemService(SENSOR_SERVICE) as SensorManager

  // Wake locks
  @Suppress("deprecation")
  val wakeLockApp = powerManager.newWakeLock(
    PowerManager.SCREEN_BRIGHT_WAKE_LOCK or PowerManager.ACQUIRE_CAUSES_WAKEUP,
    "GDApp: Active app");

  // Used for keeping track of the display timeout
  val keepDisplayOnView = View(context)
  val keepDisplayOnLayoutParams = WindowManager.LayoutParams(
    WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT,
    WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or
        WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
        WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE,
    PixelFormat.RGBA_8888
  )
  var keepDisplayOnActive = false
  var displayTimer: Timer? = null
  var displayTimeout = 0L
  var visible = false
  var forceAnimate = false
  var screenCleared = false
  var clearScreen = false

  // Indicates if an overlay window shall be used to capture gestures
  val touchHandlerView: View = View(context)
  val touchHandlerLayoutParams = WindowManager.LayoutParams(
    WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT,
    WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, 0, PixelFormat.RGBA_8888
  )
  var touchHandlerActive = false

  // Indicates that the compass provides readings
  var compassWatchStarted = false

  // Information about the OpenGL state
  var glWidth = 0
  var glHeight = 0

  // Vaiables for handling zooming via the bezel
  var zoomJob : Job? = null
  val zoomWaitTime: Long = FRAME_PERIOD_MS_DEFAULT-1
  val zoomRepeats = 10
  var zoomPos = 0

  // Variables to control fast rendering in ambient mode
  var isAmbient=false
  var isTransitioningToAmbient=false

  // Initializes everything
  init {

    // Start the core object if needed
    if (coreObject!!.coreStopped) {
      val m: Message = Message.obtain(coreObject.messageHandler)
      m.what = GDCore.START_CORE
      coreObject.messageHandler.sendMessage(m)
    }

    // Get display timeout
    // use watchDisplayTimeout from config instead of system timeout
    try {
      displayTimeout =
        Settings.System.getInt(context.contentResolver, Settings.System.SCREEN_OFF_TIMEOUT).toLong()
    } catch (e: SettingNotFoundException) {
      displayTimeout = 0
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.message)
    }

    // Setup the touch handler
    touchHandlerView.setOnTouchListener { _, event ->
      updateDisplayTimeout()
      coreObject.onTouchEvent(event)
    }
    touchHandlerView.setOnGenericMotionListener { _, motionEvent ->

      // Find out if the is a bezel event
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", event.toString());
      if (motionEvent.action == MotionEvent.ACTION_SCROLL && motionEvent.isFromSource(InputDevice.SOURCE_ROTARY_ENCODER)) {
        var startZoom = false
        var zoomValue = "1.0"
        if (motionEvent.getAxisValue(MotionEvent.AXIS_SCROLL) == 1f) {
          zoomValue = "0.96"
          startZoom = true
        }
        if (motionEvent.getAxisValue(MotionEvent.AXIS_SCROLL) == -1f) {
          zoomValue = "1.04"
          startZoom = true
        }
        if (startZoom) {

          // Do the zooming
          zoomJob?.cancel()
          var zoomEnd = zoomPos + zoomRepeats
          zoomJob=launch {
            while (zoomPos < zoomEnd) {
              /*GDApplication.addMessage(
                GDApplication.DEBUG_MSG,
                "GDApp",
                "zoomPos=${zoomPos} zoomEnd=${zoomEnd}"
              )*/
              coreObject.executeCoreCommand("zoom", zoomValue)
              delay(zoomWaitTime)
              zoomPos++
            }
            zoomPos = 0
          }
        }
      }
      updateDisplayTimeout()
      false
    }
  }

  // Forces a redraw
  fun forceRedraw() {
    forceAnimate=true
    postInvalidate()
  }

  // Enables or disable the touch handler
  fun setTouchHandlerEnabled(enable: Boolean) {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","setTouchHandlerEnabled called (enable=${enable})");
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","touchHandlerActive=${touchHandlerActive}");
    if (enable) {
      if (!touchHandlerActive) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","activating touch handler");
        windowManager.addView(touchHandlerView, touchHandlerLayoutParams)
        touchHandlerActive = true
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","touchHandlerActive=${touchHandlerActive}");
        coreObject?.executeCoreCommand("setTouchMode", "1")
      }
    } else {
      if (touchHandlerActive) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","deactivating touch handler");
        windowManager.removeView(touchHandlerView)
        touchHandlerActive = false
        coreObject?.executeCoreCommand("setTouchMode", "0")
      }
    }
  }

  // Releases the display
  @Synchronized
  fun releaseDisplay() {
    displayTimer?.cancel()
    displayTimer=null
    //windowManager.removeView(keepDisplayOnView);
    //wakeLockApp.release();
    keepDisplayOnActive = false
  }

  // Keeps the screen on for more than the default time
  @Synchronized
  fun updateDisplayTimeout() {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp", "display timeout update");
    if (displayTimeout == 0L) return
    if (displayTimer!=null)
      displayTimer?.cancel()
    displayTimer=Timer()
    if (!keepDisplayOnActive) {
      //windowManager.addView(keepDisplayOnView, keepDisplayOnLayoutParams);
      //wakeLockApp.acquire();
    }
    keepDisplayOnActive = true
    displayTimer?.schedule(object : TimerTask() {
      override fun run() {
        //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "display timeout expired")
        releaseDisplay()
      }
    }, displayTimeout)
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "ambient mode start time pushed out")
    //coreObject?.executeCoreCommand("setAmbientModeStartTime", (displayTimeout * 1000).toString())
  }

  // Handles visibility changes
  fun handleVisibility(visible: Boolean) {
    if (this.visible!=visible) {
      if (!visible) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","invisible")
        releaseDisplay();
        coreObject?.executeAppCommand("setWearDeviceSleeping(1)");
        setTouchHandlerEnabled(false);
        if (compassWatchStarted) {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","stopping compass")
          sensorManager.unregisterListener(coreObject);
          compassWatchStarted = false;
        }
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","visible")
        updateDisplayTimeout();
        if (!compassWatchStarted) {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","starting compass")
          sensorManager.registerListener(
            coreObject,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_NORMAL
          );
          sensorManager.registerListener(
            coreObject,
            sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
            SensorManager.SENSOR_DELAY_NORMAL
          );
          compassWatchStarted = true;
        }
        coreObject?.executeAppCommand("setWearDeviceSleeping(0)");
      }
      this.visible=visible
    }
  }

  // Checks if all required permissions are granted
  private fun permissionsGranted(): Boolean {
    var allPermissionsGranted = true
    if (context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) allPermissionsGranted =
      false
    if (context.checkSelfPermission(Manifest.permission.VIBRATE) != PackageManager.PERMISSION_GRANTED) allPermissionsGranted =
      false
    if (!Settings.canDrawOverlays(context)) allPermissionsGranted = false
    return allPermissionsGranted
  }

  // Called when the UI GL context is created
  override suspend fun onUiThreadGlSurfaceCreated(width: Int, height: Int) {
    super.onUiThreadGlSurfaceCreated(width, height)

    // Launch the permission dialog if needed
    if (!permissionsGranted()) {
      val intent = Intent(context, Dialog::class.java)
      intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
      intent.putExtra(Dialog.Extras.GET_PERMISSIONS, true)
      context.startActivity(intent)
    } else {

      // Set the surface info
      if ((glWidth!=width)||(glHeight!=height)) {
        coreObject?.onSurfaceCreated(null, null);
        coreObject?.onSurfaceChanged(null,width,height);
        glWidth=width
        glHeight=height
      }
    }
  }

  // Handles transition animations
  override fun shouldAnimate(): Boolean {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","visible=${watchState.isVisible.value} isAmbient=${watchState.isAmbient.value}")
    var result=super.shouldAnimate()
    if (!watchState.isVisible.value!!) {
      // If watch face becomes invisible due to other activity and later
      // becomes visible during ambient then the ambient mode is not
      // correctly started
      //handleVisibility(false)

      // Fix animation bug in WearOS 4.0
      if (!screenCleared) {
        clearScreen = true
        forceAnimate = true
      }
    }
    if (forceAnimate) {
      result=true
      forceAnimate=false
    }
    if (watchState.isAmbient.value!=null) {
      if (watchState.isAmbient.value!!) {
        if (!isAmbient) {
          //Log.d("GDApp","staring ambient at ${System.currentTimeMillis()}")
          coreObject?.executeCoreCommand("setAmbientMode", "1");
          isTransitioningToAmbient = true
        }
        isAmbient = true
        if (isTransitioningToAmbient) {
          //Log.d("GDApp","continuing drawing at ${System.currentTimeMillis()}")
          result = true
        }
      } else {
        if (isAmbient) {
          //Log.d("GDApp","stopping ambient at ${System.currentTimeMillis()}")
          coreObject?.executeCoreCommand("setAmbientMode", "0");
          isTransitioningToAmbient = false
        }
        isAmbient = false
      }
    }
    return result
  }

  // Does the drawing of the main content
  override fun render(zonedDateTime: ZonedDateTime) {
    if (renderParameters.drawMode==DrawMode.AMBIENT) {
      handleVisibility(false)
    } else {
      handleVisibility(true)
    }
    if (clearScreen) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","clearing screen")
      GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f)
      GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT)
      screenCleared=true
      clearScreen=false
    } else {
      coreObject?.onDrawFrame(null)
      screenCleared=false
    };
  }

  // Renders additional info above the main content
  override fun renderHighlightLayer(zonedDateTime: ZonedDateTime) {
  }

  // Called when the watch face is destroyed
  override fun onDestroy() {
    super.onDestroy()
    cancel()
    /*context.unregisterReceiver(drawInAmbientBroadcastReceiver)
    alarmManager.cancel(drawInAmbientPendingIntent)*/
    if ((wakeLockApp.isHeld))
      wakeLockApp.release();
    if (touchHandlerActive) {
      windowManager.removeView(touchHandlerView)
      touchHandlerActive = false
    }
    if (keepDisplayOnActive) {
      windowManager.removeView(keepDisplayOnView)
      keepDisplayOnActive = false
    }
    synchronized(activeRenderers) {
      activeRenderers.remove(this)
    }
  }

  // Called when someone touches the watch face
  override fun onTapEvent(tapType: Int, tapEvent: TapEvent, complicationSlot: ComplicationSlot?) {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("%d %d %d",tapType,x,y));
    when (tapType) {
      TapType.UP -> {
        coreObject?.executeCoreCommand("touchUp", x.toString(), y.toString())
        setTouchHandlerEnabled(true)
      }
      TapType.DOWN -> coreObject?.executeCoreCommand(
        "touchDown(",
        x.toString(),
        y.toString()
      )
      TapType.CANCEL -> coreObject?.executeCoreCommand(
        "touchCancel(",
        x.toString(),
        y.toString()
      )
    }
    updateDisplayTimeout()
  }
}

*/