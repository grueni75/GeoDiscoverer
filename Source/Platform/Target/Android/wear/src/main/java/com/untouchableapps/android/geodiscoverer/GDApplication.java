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

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.io.File;

/* Main application class */
public class GDApplication extends Application implements GDAppInterface {

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;

  /** Reference to the viewmap activity */
  ViewMap activity = null;

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
  public static void showMessageBar(Activity activity, String message, long duration) {
    Toast.makeText(activity.getApplicationContext(),message,(int)duration).show();
  }

  /** Sets the view map activity */
  public void setActivity(ViewMap activity) {
    this.activity = activity;
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
    if (activity != null) {
      Message m = Message.obtain(activity.coreMessageHandler);
      m.what = ViewMap.EXECUTE_COMMAND;
      Bundle b = new Bundle();
      b.putString("command", cmd);
      m.setData(b);
      activity.coreMessageHandler.sendMessage(m);
    }
  }

  // Returns the orientation of the activity
  @Override
  public int getActivityOrientation() {
    if (activity!=null) {
      return activity.getResources().getConfiguration().orientation;
    } else {
      return Configuration.ORIENTATION_UNDEFINED;
    }
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

}
