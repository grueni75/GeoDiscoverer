//============================================================================
// Name        : GDApplication.java
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

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.wearable.MessageApi;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Wearable;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.io.File;

/* Main application class */
public class GDApplication extends Application implements GDAppInterface {

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;

  /** Reference to the message handler of the watch face */
  WatchFace.CoreMessageHandler messageHandler = null;

  /** Called when the application starts */
  @Override
  public void onCreate() {
    super.onCreate();  

    // Initialize the core object
    String homeDirPath = GDCore.getHomeDirPath();
    if (homeDirPath.equals("")) {
      Toast.makeText(this, String.format(String.format(getString(R.string.no_home_dir),homeDirPath)), Toast.LENGTH_LONG).show();
      System.exit(1);
    } else {
      coreObject=new GDCore(this, homeDirPath);
    }
  }

  /** Adds a message to the log */
  public static synchronized void addMessage(int severityNumber, String tag, String message) {
    String severityString;
    switch (severityNumber) {
      case DEBUG_MSG: severityString="DEBUG"; if (!tag.equals("GDCore")) Log.d(tag, message); break;
      case INFO_MSG: severityString="INFO";  if (!tag.equals("GDCore")) Log.i(tag, message); break;      
      case WARNING_MSG: severityString="WARNING"; if (!tag.equals("GDCore")) Log.w(tag, message); break;
      case ERROR_MSG: severityString="ERROR"; if (!tag.equals("GDCore")) Log.e(tag, message); break;   
      case FATAL_MSG: severityString="FATAL"; if (!tag.equals("GDCore")) Log.e(tag, message); break;
      default: severityString="UNKNOWN"; if (!tag.equals("GDCore")) Log.e(tag, message); break;
    }
    if (coreObject!=null) {
      if (!tag.equals("GDCore")) {
        coreObject.executeCoreCommand("log(" + severityString + "," + tag + "," + message + ")");
      }
    }
  }

  public static final long MESSAGE_BAR_DURATION_SHORT = 2000;
  public static final long MESSAGE_BAR_DURATION_LONG = 4000;
  
  /** Shows a toast */
  public static void showMessageBar(Context context, String message, long duration) {
    Toast.makeText(context,message,(int)duration).show();
  }

  /** Sets the message handler */
  public void setMessageHandler(WatchFace.CoreMessageHandler messageHandler) {
    this.messageHandler = messageHandler;
  }

  // Cockpit engine related interface methods
  @Override
  public void cockpitEngineStart() {
  }
  @Override
  public void cockputEngineUpdate(String infos) {
  }
  @Override
  public void cockpitEngineStop() {
  }
  @Override
  public boolean cockpitEngineIsActive() {
    return false;
  }

  // Sends a native crash report
  @Override
  public void sendNativeCrashReport(String dumpBinPath, boolean quitApp) {
    addMessage(FATAL_MSG,"GDApp","Native crash occured in GDCore!");
  }

  // Returns the application context
  @Override
  public Context getContext() {
    return getApplicationContext();
  }

  // Sends a command to the activity
  @Override
  public void executeAppCommand(String cmd) {
    if (messageHandler != null) {
      Message m = Message.obtain(messageHandler);
      m.what = WatchFace.EXECUTE_COMMAND;
      Bundle b = new Bundle();
      b.putString("command", cmd);
      m.setData(b);
      messageHandler.sendMessage(m);
    }
  }

  // Returns the orientation of the activity
  @Override
  public int getActivityOrientation() {
    return Configuration.ORIENTATION_PORTRAIT;
  }

  // Returns an intent for the GDService
  @Override
  public Intent createServiceIntent() {
    return null;
  }

  // Adds a message
  @Override
  public void addAppMessage(int severityNumber, String tag, String message) {
    GDApplication.addMessage(severityNumber,tag,message);
  }

  // Returns the application
  @Override
  public Application getApplication() {
    return this;
  }

  // Sends a command to the wear device
  public void sendWearCommand( final String command ) {
  }

}
