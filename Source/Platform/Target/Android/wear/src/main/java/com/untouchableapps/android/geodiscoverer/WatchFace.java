//============================================================================
// Name        : WatchFace.java
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

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.opengl.GLES20;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings;
import android.support.wearable.watchface.Gles2WatchFaceService;
import android.support.wearable.watchface.WatchFaceService;
import android.support.wearable.watchface.WatchFaceStyle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.lang.ref.WeakReference;
import java.util.Vector;

public class WatchFace extends Gles2WatchFaceService {

  /** Reference to the core object */
  GDCore coreObject = null;

  // Managers
  PowerManager powerManager = null;
  WindowManager windowManager = null;
  LayoutInflater inflater = null;

  // Indicates if the dialog is open
  boolean dialogVisible = false;

  // Wake lock
  PowerManager.WakeLock wakeLock = null;

  // Types of dialogs
  static final int FATAL_DIALOG = 0;
  static final int ERROR_DIALOG = 2;
  static final int WARNING_DIALOG = 1;
  static final int INFO_DIALOG = 3;

  /** Time the last toast was shown */
  long lastToastTimestamp = 0;

  /** Minimum distance between two toasts in milliseconds */
  int toastDistance = 5000;

  /** Shows a dialog  */
  public synchronized void dialog(int kind, String message) {
    if (kind==WARNING_DIALOG) {
      long diff= SystemClock.uptimeMillis()-lastToastTimestamp;
      if (diff<=toastDistance) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping dialog request <" + message + "> because toast is still visible");
        return;
      }
      GDApplication.showMessageBar(getApplicationContext(), message, GDApplication.MESSAGE_BAR_DURATION_LONG);
      lastToastTimestamp=SystemClock.uptimeMillis();
    } else if (kind==INFO_DIALOG) {
      GDApplication.showMessageBar(getApplicationContext(), message, GDApplication.MESSAGE_BAR_DURATION_LONG);
    } else {
      Intent intent = new Intent(this, Dialog.class);
      intent.putExtra(Dialog.EXTRA_TEXT, message);
      intent.putExtra(Dialog.EXTRA_KIND, kind);
      startActivity(intent);
    }
  }

  /** Shows a fatal dialog and quits the applications */
  public void fatalDialog(String message) {
    dialog(FATAL_DIALOG,message);
  }

  /** Shows an error dialog without quitting the application */
  public void errorDialog(String message) {
    dialog(ERROR_DIALOG,message);
  }

  /** Shows a warning dialog without quitting the application */
  public void warningDialog(String message) {
    dialog(WARNING_DIALOG, message);
  }

  /** Shows an info dialog without quitting the application */
  public void infoDialog(String message) {
    dialog(INFO_DIALOG,message);
  }

  // Variables for monitoring the state of the external storage
  boolean externalStorageAvailable = false;
  boolean externalStorageWritable = false;

  /** Last created engine */
  Engine lastEngine = null;

  /** Sets the screen time out */
  @SuppressLint("Wakelock")
  void updateWakeLock() {
    String state=coreObject.executeCoreCommand("getWakeLock()");
    if (state.equals("true")) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","wake lock enabled");
      if (!wakeLock.isHeld())
        wakeLock.acquire();
    } else {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","wake lock disabled");
      if (wakeLock.isHeld())
        wakeLock.release();
    }
  }

  // Communication with the native core
  public static final int EXECUTE_COMMAND = 0;
  static protected class CoreMessageHandler extends Handler {

    protected final WeakReference<WatchFace> weakWatchFace;

    CoreMessageHandler(WatchFace watchFace) {
      this.weakWatchFace = new WeakReference<WatchFace>(watchFace);
    }

    /** Called when the core has a message */
    @SuppressLint("NewApi")
    @Override
    public void handleMessage(Message msg) {

      // Abort if the object is not available anymore
      WatchFace watchFace = weakWatchFace.get();
      if (watchFace==null)
        return;

      // Handle the message
      Bundle b=msg.getData();
      switch(msg.what) {
        case EXECUTE_COMMAND:

          // Extract the command
          String command=b.getString("command");
          int args_start=command.indexOf("(");
          int args_end=command.lastIndexOf(")");
          String commandFunction=command.substring(0, args_start);
          String t=command.substring(args_start+1,args_end);
          Vector<String> commandArgs = new Vector<String>();
          boolean stringStarted=false;
          int startPos=0;
          for(int i=0;i<t.length();i++) {
            if (t.substring(i,i+1).equals("\"")) {
              if (stringStarted)
                stringStarted=false;
              else
                stringStarted=true;
            }
            if (!stringStarted) {
              if ((t.substring(i,i+1).equals(","))||(i==t.length()-1)) {
                String arg;
                if (i==t.length()-1)
                  arg=t.substring(startPos,i+1);
                else
                  arg=t.substring(startPos,i);
                if (arg.startsWith("\"")) {
                  arg=arg.substring(1);
                }
                if (arg.endsWith("\"")) {
                  arg=arg.substring(0,arg.length()-1);
                }
                commandArgs.add(arg);
                startPos=i+1;
              }
            }
          }

          // Execute command
          boolean commandExecuted=false;
          if (commandFunction.equals("fatalDialog")) {
            watchFace.fatalDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("errorDialog")) {
            watchFace.errorDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("warningDialog")) {
            watchFace.warningDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("infoDialog")) {
            watchFace.infoDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("createProgressDialog")) {

            // Create a new dialog if it does not yet exist
            if (!watchFace.dialogVisible) {
              Intent intent = new Intent(watchFace, Dialog.class);
              intent.putExtra(Dialog.EXTRA_TEXT, commandArgs.get(0));
              int max = Integer.parseInt(commandArgs.get(1));
              intent.putExtra(Dialog.EXTRA_MAX, max);
              watchFace.startActivity(intent);
              watchFace.dialogVisible=true;
            } else {
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping progress dialog request <" + commandArgs.get(0) + "> because progress dialog is already visible");
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("updateProgressDialog")) {
            if (watchFace.dialogVisible) {
              Intent intent = new Intent(watchFace, Dialog.class);
              intent.putExtra(Dialog.EXTRA_PROGRESS, Integer.parseInt(commandArgs.get(1)));
              watchFace.startActivity(intent);
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("closeProgressDialog")) {
            if (watchFace.dialogVisible) {
              Intent intent = new Intent(watchFace, Dialog.class);
              intent.putExtra(Dialog.EXTRA_CLOSE, true);
              watchFace.startActivity(intent);
              watchFace.dialogVisible=false;
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("coreInitialized")) {
            // Nothing to do as of now
            commandExecuted=true;
          }
          if (commandFunction.equals("lateInitComplete")) {
            // Nothing to do as of now
            commandExecuted=true;
          }
          if (commandFunction.equals("getLastKnownLocation")) {
            // Nothing to do as of now
            commandExecuted=true;
          }
          if (commandFunction.equals("setSplashVisibility")) {
            // Nothing to do as of now
            commandExecuted=true;
          }
          if (commandFunction.equals("updateWakeLock")) {
            watchFace.updateWakeLock();
            commandExecuted=true;
          }
          if (commandFunction.equals("updateScreen")) {
            if (watchFace.lastEngine!=null) {
              //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","invalidating screen");
              watchFace.lastEngine.invalidate();
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("restartActivity")) {
            if (GDApplication.coreObject!=null) {
              Message m = Message.obtain(GDApplication.coreObject.messageHandler);
              m.what = GDCore.START_CORE;
              GDApplication.coreObject.messageHandler.sendMessage(m);
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("exitActivity")) {
            commandExecuted=true;
          }
          if (!commandExecuted) {
            GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "unknown command " + command + " received");
          }
          break;
      }
    }
  }
  CoreMessageHandler coreMessageHandler = new CoreMessageHandler(this);

  @Override
  public void onCreate() {
    super.onCreate();

    // Get important handles
    powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
    windowManager = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
    inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);

    // Get a wake lock
    if (wakeLock!=null)
      wakeLock.release();
    wakeLock=powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "ActiveCPU");
    if (wakeLock==null) {
      fatalDialog("Can not obtain wake lock!");
    }

    // Check for OpenGL ES 2.00
    final ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
    final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
    final boolean supportsEs2 = (configurationInfo.reqGlEsVersion >= 0x20000) || (Build.FINGERPRINT.startsWith("generic"));
    if (!supportsEs2)
    {
      fatalDialog(getString(R.string.opengles20_required));
      return;
    }

    // Only start the core object if all permissions are available
    if (permissionsGranted()) {

      // Get the core object
      coreObject = GDApplication.coreObject;
      ((GDApplication) getApplication()).setMessageHandler(coreMessageHandler);

      // Start the core object
      Message m=Message.obtain(coreObject.messageHandler);
      m.what = GDCore.START_CORE;
      coreObject.messageHandler.sendMessage(m);

    }
  }

  /** Checks if all required permissions are granted */
  private boolean permissionsGranted() {
    boolean allPermissionsGranted=true;
    if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
      allPermissionsGranted=false;
    if (checkSelfPermission(Manifest.permission.VIBRATE) != PackageManager.PERMISSION_GRANTED)
      allPermissionsGranted=false;
    return allPermissionsGranted;
  }

  @Override
  public Engine onCreateEngine() {
    lastEngine = new Engine();
    return lastEngine;
  }

  /** Called when the activity is destroyed */
  @Override
  public void onDestroy() {
    super.onDestroy();
    ((GDApplication)getApplication()).setMessageHandler(null);
    if ((wakeLock!=null)&&(wakeLock.isHeld()))
      wakeLock.release();
  }

  private class Engine extends Gles2WatchFaceService.Engine {

    BroadcastReceiver screenStateReceiver = null;

    @Override
    public void onCreate(SurfaceHolder surfaceHolder) {
      super.onCreate(surfaceHolder);

      // Set the watch face style
      setWatchFaceStyle(new WatchFaceStyle.Builder(WatchFace.this)
          .setCardPeekMode(WatchFaceStyle.PEEK_MODE_SHORT)
          .setAmbientPeekMode(WatchFaceStyle.AMBIENT_PEEK_MODE_HIDDEN)
          .setBackgroundVisibility(WatchFaceStyle.BACKGROUND_VISIBILITY_INTERRUPTIVE)
          .setStatusBarGravity(Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM)
          .setHotwordIndicatorGravity(Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM)
          .setShowSystemUiTime(false)
          .setShowUnreadCountIndicator(false)
          .setViewProtectionMode(WatchFaceStyle.PROTECT_HOTWORD_INDICATOR | WatchFaceStyle.PROTECT_STATUS_BAR)
          .setAcceptsTapEvents(true)
          .build());

      // Get informed if screen is turned off
      if (coreObject!=null) {
        if (powerManager.isInteractive())
          coreObject.executeAppCommand("setWearDeviceSleeping(0)");
        else
          coreObject.executeAppCommand("setWearDeviceSleeping(1)");
      }
      IntentFilter intentFilter = new IntentFilter(Intent.ACTION_SCREEN_ON);
      intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
      screenStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          if (coreObject==null)
            return;
          if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
            coreObject.executeAppCommand("setWearDeviceSleeping(1)");
          } else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
            coreObject.executeAppCommand("setWearDeviceSleeping(0)");
          }
        }
      };
      registerReceiver(screenStateReceiver, intentFilter);

      // Inform the user that permissions must be granted
      if (coreObject==null) {
        Intent intent = new Intent(getApplication(), Dialog.class);
        intent.putExtra(Dialog.EXTRA_TEXT, getResources().getString(R.string.permission_instructions));
        intent.putExtra(Dialog.EXTRA_KIND, ERROR_DIALOG);
        intent.putExtra(Dialog.EXTRA_GET_PERMISSIONS, true);
        startActivity(intent);
      }
    }

    @Override
    public void onDestroy() {
      if (coreObject!=null)
        coreObject.executeAppCommand("setWearDeviceAlive(0)");
      unregisterReceiver(screenStateReceiver);
      if (coreObject!=null) {
        Message m = Message.obtain(coreObject.messageHandler);
        m.what = GDCore.STOP_CORE;
        coreObject.messageHandler.sendMessage(m);
      }
    }

    @Override
    public void onGlContextCreated() {
      super.onGlContextCreated();
      if (coreObject==null) return;
      coreObject.onSurfaceCreated(null,null);
    }

    @Override
    public void onGlSurfaceCreated(int width, int height) {
      super.onGlSurfaceCreated(width,height);
      if (coreObject==null) return;
      coreObject.onSurfaceChanged(null,width,height);
    }

    @Override
    public void onAmbientModeChanged(boolean inAmbientMode) {
      super.onAmbientModeChanged(inAmbientMode);
      invalidate();
      if (coreObject==null) return;
      if (inAmbientMode)
        coreObject.executeAppCommand("setWearDeviceSleeping(1)");
      else
        coreObject.executeAppCommand("setWearDeviceSleeping(0)");
    }

    @Override
    public void onVisibilityChanged(boolean visible) {
      super.onVisibilityChanged(visible);
      if (visible) {
        invalidate();
      }
    }
    @Override
    public void onTimeTick() {
      super.onTimeTick();
      invalidate();
    }

    @Override
    public void onDraw() {
      super.onDraw();

      //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","draw!");

      // In ambient mode, do not draw
      if (isInAmbientMode()) {

        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

      } else {

        // Let the core object draw
        if (coreObject!=null)
          coreObject.onDrawFrame(null);
        else {
          if (permissionsGranted())
            System.exit(0);
          GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
          GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        }
      }

      // Draw every frame as long as we're visible and in interactive mode.
      if (isVisible() && !isInAmbientMode()) {
        invalidate();
      }
    }

    @Override
    public void onTapCommand(@TapType int tapType, int x, int y, long eventTime) {
      if (coreObject==null) return;
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("%d %d %d",tapType,x,y));
      switch (tapType) {
        case WatchFaceService.TAP_TYPE_TAP:
          coreObject.executeCoreCommand("touchUp(" + x + "," + y + ")");
          break;

        case WatchFaceService.TAP_TYPE_TOUCH:
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");
          break;

        case WatchFaceService.TAP_TYPE_TOUCH_CANCEL:
          coreObject.executeCoreCommand("touchCancel(" + x + "," + y + ")");
          break;

        default:
          super.onTapCommand(tapType, x, y, eventTime);
          break;
      }
    }
  }
}