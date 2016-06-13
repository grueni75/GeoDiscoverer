//============================================================================
// Name        : WatchFace.java
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

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.graphics.PixelFormat;
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
import android.support.wearable.watchface.WatchFaceStyle;
import android.view.Gravity;
import android.view.LayoutInflater;
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

  // Dialog window
  FrameLayout dialogLayout = null;
  TextView progressDialogText;
  ProgressBar progressDialogBar;
  LinearLayout progressDialogLayout;
  TextView messageDialogText;
  ImageButton messageDialogImageButton;
  LinearLayout messageDialogLayout;

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
      if (dialogLayout==null) {
        dialogLayout = (FrameLayout) inflater.inflate(R.layout.dialog, null);
        messageDialogText = (TextView) dialogLayout.findViewById(R.id.message_dialog_text);
        messageDialogImageButton = (ImageButton) dialogLayout.findViewById(R.id.message_dialog_button);
        messageDialogLayout = (LinearLayout) dialogLayout.findViewById(R.id.message_dialog);
        WindowManager.LayoutParams params =  new WindowManager.LayoutParams(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.TYPE_SYSTEM_ALERT, 0, PixelFormat.TRANSLUCENT);
        windowManager.addView(dialogLayout,params);
        messageDialogText.setText(message);
        if (kind == FATAL_DIALOG) {
          messageDialogImageButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              windowManager.removeView(dialogLayout);
              dialogLayout=null;
              System.exit(1);
            }
          });
        } else {
          messageDialogImageButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              windowManager.removeView(dialogLayout);
              dialogLayout=null;
            }
          });
        }
        messageDialogLayout.setVisibility(messageDialogLayout.VISIBLE);
        dialogLayout.requestLayout();
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping dialog request <" + message + "> because alert dialog is already visible");
      }
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

  /** Called when the external storage state changes */
  synchronized void handleExternalStorageState() {

    // If the external storage is not available, inform the user that this will not work
    if ((!externalStorageAvailable)||(!externalStorageWritable)) {
      GDApplication.addMessage(GDAppInterface.ERROR_MSG, "GDApp", String.format(getString(R.string.no_external_storage)));
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
            //commandExecuted=true;
          }
          if (commandFunction.equals("errorDialog")) {
            watchFace.errorDialog(commandArgs.get(0));
            //commandExecuted=true;
          }
          if (commandFunction.equals("warningDialog")) {
            watchFace.warningDialog(commandArgs.get(0));
            //commandExecuted=true;
          }
          if (commandFunction.equals("infoDialog")) {
            watchFace.infoDialog(commandArgs.get(0));
            //commandExecuted=true;
          }
          if (commandFunction.equals("createProgressDialog")) {

            // Create a new dialog if it does not yet exist
            if (watchFace.dialogLayout==null) {
              watchFace.dialogLayout = (FrameLayout) watchFace.inflater.inflate(R.layout.dialog, null);
              watchFace.progressDialogText = (TextView) watchFace.dialogLayout.findViewById(R.id.progress_dialog_text);
              watchFace.progressDialogBar = (ProgressBar) watchFace.dialogLayout.findViewById(R.id.progress_dialog_bar);
              watchFace.progressDialogLayout = (LinearLayout) watchFace.dialogLayout.findViewById(R.id.progress_dialog);
              WindowManager.LayoutParams params =  new WindowManager.LayoutParams(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.TYPE_SYSTEM_ALERT, 0, PixelFormat.TRANSLUCENT);
              watchFace.windowManager.addView(watchFace.dialogLayout,params);
              watchFace.progressDialogText.setText(commandArgs.get(0));
              int max=Integer.parseInt(commandArgs.get(1));
              watchFace.progressDialogBar.setIndeterminate(max==0?true:false);
              watchFace.progressDialogBar.setMax(max);
              watchFace.progressDialogBar.setProgress(0);
              watchFace.progressDialogLayout.setVisibility(watchFace.progressDialogLayout.VISIBLE);
              watchFace.dialogLayout.requestLayout();
            } else {
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping progress dialog request <" + commandArgs.get(0) + "> because progress dialog is already visible");
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("updateProgressDialog")) {
            watchFace.progressDialogText.setText(commandArgs.get(0));
            int progress=Integer.parseInt(commandArgs.get(1));
            watchFace.progressDialogBar.setProgress(progress);
            watchFace.dialogLayout.requestLayout();
            commandExecuted=true;
          }
          if (commandFunction.equals("closeProgressDialog")) {
            watchFace.windowManager.removeView(watchFace.dialogLayout);
            watchFace.dialogLayout=null;
            commandExecuted=true;
          }
          if (commandFunction.equals("coreInitialized")) {
            // Nothing to do as of now
            commandExecuted=true;
          }
          if (commandFunction.equals("initComplete")) {
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

    // Get the core object
    coreObject=GDApplication.coreObject;
    coreObject.setDisplayMetrics(getResources().getDisplayMetrics());
    ((GDApplication)getApplication()).setMessageHandler(coreMessageHandler);

    // Check for external storage
    updateExternalStorageState();
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
          .setShowUnreadCountIndicator(true)
          .setViewProtectionMode(WatchFaceStyle.PROTECT_HOTWORD_INDICATOR | WatchFaceStyle.PROTECT_STATUS_BAR)
          .build());
    }

    @Override
    public void onDestroy() {
      if (dialogLayout!=null)
        windowManager.removeView(dialogLayout);
      Message m=Message.obtain(coreObject.messageHandler);
      m.what = GDCore.STOP_CORE;
      coreObject.messageHandler.sendMessage(m);
    }

    @Override
    public void onGlContextCreated() {
      super.onGlContextCreated();
      coreObject.onSurfaceCreated(null,null);
    }

    @Override
    public void onGlSurfaceCreated(int width, int height) {
      super.onGlSurfaceCreated(width,height);
      coreObject.onSurfaceChanged(null,width,height);
    }

    @Override
    public void onAmbientModeChanged(boolean inAmbientMode) {
      super.onAmbientModeChanged(inAmbientMode);
      invalidate();
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

      // Let the core object draw
      coreObject.onDrawFrame(null);

      // Draw every frame as long as we're visible and in interactive mode.
      if (isVisible() && !isInAmbientMode()) {
        invalidate();
      }
    }
  }
}