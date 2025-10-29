//============================================================================
// Name        : GDApplication.java
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

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.tasks.Tasks;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.Wearable;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.GDTools;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitEngine;
import com.untouchableapps.android.geodiscoverer.ui.CoreMessageHandler;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/* Main application class */
public class GDApplication extends Application implements GDAppInterface {

  // Notification IDs
  static public final int NOTIFICATION_STATUS_ID=1;

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;

  /** Reference to the message handler of the watch face */
  CoreMessageHandler messageHandler = null;

  /** Cockpit engine */
  CockpitEngine cockpitEngine = null;

  /** List of commands to send to wear */
  LinkedBlockingQueue<String> wearCommands = new LinkedBlockingQueue<String>();

  /** Thread that sends commands */
  Thread wearThread = null;

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

    // Start the thread that handles the wear communication
    wearThread = new Thread(new Runnable() {
      @Override
      public void run() {
        while (true) {
          try {

            // Get command
            String command = wearCommands.take();
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","processing wear command: " + command);

            // Send command
            CapabilityInfo capabilityInfo = Tasks.await(
                Wearable.getCapabilityClient(getApplicationContext()).getCapability(
                    GDTools.WEAR_CAPABILITY_NAME, CapabilityClient.FILTER_REACHABLE),
                GDTools.wearTransmissionTimout, TimeUnit.MILLISECONDS);
            String nodeId = GDTools.pickBestWearNodeId(capabilityInfo.getNodes());
            //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "found node id: " + nodeId);
            if (nodeId!=null) {
              Wearable.getMessageClient(getApplicationContext()).sendMessage(
                  nodeId, "/com.untouchableapps.android.geodiscoverer", command.getBytes()
              );
            }
          }
          catch (InterruptedException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
          catch (ExecutionException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
          catch (TimeoutException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","phone not reachable");
          }
        }
      }
    });
    wearThread.start();
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
        coreObject.executeCoreCommand("log",severityString,tag,message);
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
  public void setMessageHandler(CoreMessageHandler messageHandler) {
    this.messageHandler = messageHandler;
  }

  // Cockpit engine related interface methods
  @Override
  public void cockpitEngineStart() {
    if (cockpitEngine!=null)
      cockpitEngineStop();
    cockpitEngine=new CockpitEngine(getContext(),coreObject);
    cockpitEngine.start();
  }
  @Override
  public void cockputEngineUpdate(String infos) {
    if (cockpitEngine!=null)
      cockpitEngine.update(infos, false);

  }
  @Override
  public void cockpitEngineStop() {
    if (cockpitEngine!=null) {
      cockpitEngine.stop();
      cockpitEngine=null;
    }
  }
  @Override
  public boolean cockpitEngineIsActive() {
    if (cockpitEngine!=null) {
      return cockpitEngine.isActive();
    } else {
      return false;
    }
  }

  // Sends a native crash report
  @Override
  public void sendNativeCrashReport(String dumpBinPath, boolean quitApp) {
    addMessage(FATAL_MSG,"GDApp","Native crash occured in GDCore!");
  }

  // Sends a native crash report
  @Override
  public void scheduleRestart() {
    addMessage(FATAL_MSG,"GDApp","Schedule restart is not supported!");
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
      m.what = 0;
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
    wearCommands.offer(command);
  }

}
