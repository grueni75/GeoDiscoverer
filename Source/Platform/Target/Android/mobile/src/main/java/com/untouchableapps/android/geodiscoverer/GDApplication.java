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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import org.acra.*;
import org.acra.annotation.*;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v4.app.FragmentActivity;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.Api;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.Status;
import com.google.android.gms.wearable.Channel;
import com.google.android.gms.wearable.ChannelApi;
import com.google.android.gms.wearable.MessageApi;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.NodeApi;
import com.google.android.gms.wearable.Wearable;
import com.untouchableapps.android.geodiscoverer.cockpit.CockpitEngine;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitAppVibration;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitInfos;

/* Configuration of ACRA for reporting crashes */
@AcraCore (
    buildConfigClass = BuildConfig.class
)
@AcraMailSender(
    mailTo = "geodiscoverer@gmail.com",
    resSubject = R.string.crash_mail_subject
)
@AcraNotification(
    resTickerText = R.string.crash_notification_ticker_text,
    resTitle = R.string.crash_notification_title,
    resText = R.string.crash_notification_text,
    resChannelName = R.string.crash_notification_channel
)

/* Main application class */
public class GDApplication extends Application implements GDAppInterface, GoogleApiClient.ConnectionCallbacks, ChannelApi.ChannelListener {

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;
  
  /** Message log */
  public static String messages = "";
  
  /** Maximum size of the message log */
  final static int maxMessagesSize = 4096;

  /** Dashboard network service type */
  public static final String dashboardNetworkServiceType = "_geodashboard._tcp.local.";

  /** Cockpit engine */
  CockpitEngine cockpitEngine = null;

  /** Reference to the viewmap activity */
  ViewMap activity = null;

  /** Time to wait for a connection */
  final static long WEAR_CONNECTION_TIME_OUT_MS = 1000;

  /** List of commands to send to wear */
  LinkedBlockingQueue<String> wearCommands = new LinkedBlockingQueue<String>();

  /** Thread that sends commands */
  Thread wearThread = null;

  /** Indicates if wear device is not shwoing anything */
  private boolean wearDeviceSleeping = true;

  /** Indicates if wear device is active (watch face visible) */
  private boolean wearDeviceAlive = false;

  /** Last command the wear thread has sent */
  String wearLastCommand="";

  /** Attachment that shall be sent to the remote side */
  File attachment = null;
  boolean attachmentSent;

  /** Init Acra */
  @Override
  protected void attachBaseContext(Context base) {
    super.attachBaseContext(base);

    // The following line triggers the initialization of ACRA
    ACRA.init(this);
  }

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
    
    // Send any left over native crash reports
    File logDir = new File(homeDirPath + "/Log");
    File[] files = logDir.listFiles();
    if (files!=null) {
      for (File file : files) {
        if ((file.isFile()) && (file.getAbsolutePath().endsWith(".dmp"))) {
          String txtPath = file.getAbsolutePath();
          txtPath = txtPath.substring(0,txtPath.length()-4) + ".zip.base64";
          File file2 = new File(txtPath);
          if (!file2.exists()) {
            sendNativeCrashReport(file.getAbsolutePath(), true);
            break; // only one report at a time
          }
        }
      }
    }

    // Init the message api to communicate with wear device
    Wearable.ChannelApi.addListener(coreObject.googleApiClient, this);

    // Start the thread that handles the wear communication
    wearThread = new Thread(new Runnable() {
      @Override
      public void run() {
        long lastUpdate=0;
        while (true) {
          try {

            // Get command
            String command = wearCommands.take();
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","processing wear command: " + command);

            // Only process command if device is alive
            if ((wearDeviceAlive)||(command.equals("getWearDeviceAlive()"))) {

              // Check if it is necessary to send command
              long t=System.currentTimeMillis();
              boolean sendMessage = true;
              
              // Handle attachments
              if (command.startsWith("serveRemoteMapArchive")) {
                String path = command.substring(command.indexOf("(")+1, command.indexOf(")"));
                attachment = new File(path);
              }

              // Send command
              if (sendMessage) {
                //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "command=" + command);
                lastUpdate = t;
                NodeApi.GetConnectedNodesResult nodes =
                    Wearable.NodeApi.getConnectedNodes(coreObject.googleApiClient).await(WEAR_CONNECTION_TIME_OUT_MS,
                        TimeUnit.MILLISECONDS);
                if (nodes != null) {
                  for (Node node : nodes.getNodes()) {
                    if (attachment!=null) {
                      ChannelApi.OpenChannelResult result = Wearable.ChannelApi.openChannel(coreObject.googleApiClient, node.getId(), "/com.untouchableapps.android.geodiscoverer/mapArchive" + attachment.getAbsolutePath()).await();
                      Channel channel = result.getChannel();
                      attachmentSent=false;
                      channel.sendFile(coreObject.googleApiClient, Uri.fromFile(attachment));
                      synchronized (attachment) {
                        while (!attachmentSent)
                          attachment.wait();
                      }
                      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "file " + attachment.getAbsolutePath() + " sent to remote device");
                      synchronized (attachment) {
                        attachment=null;
                      }
                    } else {
                      MessageApi.SendMessageResult result = Wearable.MessageApi.sendMessage(
                          coreObject.googleApiClient, node.getId(), "/com.untouchableapps.android.geodiscoverer",
                          command.getBytes()).await(WEAR_CONNECTION_TIME_OUT_MS, TimeUnit.MILLISECONDS);
                    }
                  }
                }
                wearLastCommand="";
              } else {
                wearLastCommand=command;
                //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "lastCommand=" + wearLastCommand);
              }
            }
          }
          catch (InterruptedException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
        }
      }
    });
    wearThread.start();
  }

  /** Called when a connection to a wear device has been established */
  @Override
  public void onConnected(Bundle bundle) {
  }

  /** Called when a connection to a wear device has been shut down */
  @Override
  public void onConnectionSuspended(int cause) {
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
    messages = messages + String.format("\n%-15s %s", severityString + "[" + tag + "]:", message);
    if (messages.length()>maxMessagesSize) {
      messages = messages.substring(messages.length() - maxMessagesSize);
    }
    if (coreObject!=null) {
      if (!tag.equals("GDCore")) {
        coreObject.executeCoreCommand("log(" + severityString + "," + tag + "," + message + ")");
      }
      coreObject.executeAppCommand("updateMessages()");
    }
  }

  public static final long MESSAGE_BAR_DURATION_SHORT = 2000;
  public static final long MESSAGE_BAR_DURATION_LONG = 4000;
  
  /** Shows a toast */
  public static void showMessageBar(Activity activity, String message, long duration) {
    View v = activity.getWindow().getDecorView().findViewById(R.id.view_map_snackbar_position);
    if (v==null) {
      v = activity.getWindow().getDecorView().findViewById(android.R.id.content);
    }
    Snackbar
    .make(v, message, Snackbar.LENGTH_LONG)
    .show(); // Don’t forget to show!
  }

  /** Sets the view map activity */
  public void setActivity(ViewMap activity) {
    this.activity = activity;
  }

  // Cockpit engine related interface methods
  @Override
  public void cockpitEngineStart() {
    if (cockpitEngine!=null)
      cockpitEngineStop();
    cockpitEngine=new CockpitEngine(this);
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

  // Adds a file to a zip archive
  void zipFile(ZipOutputStream zipOut, String name, String path) throws IOException {
    FileInputStream in = new FileInputStream(path);
    zipOut.putNextEntry(new ZipEntry(name));
    byte[] b = new byte[1024];
    int count;
    while ((count = in.read(b)) > 0) {
      zipOut.write(b, 0, count);
    }
    in.close();
  }

  // Sends a native crash report
  @Override
  public void sendNativeCrashReport(String dumpBinPath, boolean quitApp) {

    // Get the content of the minidump file and format it in base64
    String basePath = dumpBinPath.substring(0,dumpBinPath.length()-4);
    String zipPath = basePath.concat(".zip");
    String logsPath = basePath.concat(".logs");
    ACRA.getErrorReporter().putCustomData("nativeZipPath", zipPath);
    String txtPath = zipPath.concat(".base64");
    try {

      // Create a zip file with the dump and all reported logs
      ZipOutputStream zipOut = new ZipOutputStream(new FileOutputStream(zipPath));
      zipFile(zipOut,"crash.dmp",dumpBinPath);
      File logsFile = new File(logsPath);
      if (logsFile.exists()) {
        String logPath;
        InputStream fis = new FileInputStream(logsPath);
        InputStreamReader isr = new InputStreamReader(fis);
        BufferedReader br = new BufferedReader(isr);
        while ((logPath = br.readLine()) != null) {
          File logFile = new File(logPath);
          if (logFile.exists())
            zipFile(zipOut, logFile.getName(), logPath);
        }
      }
      zipOut.close();

      // Submit the zip file after text encoding
      DataInputStream binReader = new DataInputStream(new FileInputStream(zipPath));
      long len = new File(zipPath).length();
      if (len > Integer.MAX_VALUE) {
        binReader.close();
        throw new IOException("File "+dumpBinPath+" too large, was "+len+" bytes.");
      }
      byte[] bytes = new byte[(int) len];
      binReader.readFully(bytes);
      String contents = Base64.encodeToString(bytes, Base64.DEFAULT);
      BufferedWriter textWriter = new BufferedWriter(new FileWriter(txtPath));
      textWriter.write(contents);
      textWriter.close();
      ACRA.getErrorReporter().putCustomData("nativeZipContents",contents);
      binReader.close();
    }
    catch (Exception e) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
    }

    // Send report via ACRA
    Exception e = new Exception("GDCore has crashed");
    ACRA.getErrorReporter().handleException(e,quitApp);
  }

  // Returns the application context
  @Override
  public Context getContext() {
    return getApplicationContext();
  }

  // Sends a command to the activity
  @Override
  public void executeAppCommand(String cmd) {
    if (cmd.equals("lateInitComplete()")) {

      // Inform the service
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("lateInitComplete");
      startService(intent);

      // Ask wear device for current power status
      sendWearCommand("getWearDeviceAlive()");
    }
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
    return new Intent(getApplicationContext(), GDService.class);
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

  // Informs the application about the wear sleep state
  public void setWearDeviceSleeping(boolean state) {
    wearDeviceSleeping=state;
    if ((wearDeviceAlive)&&(!wearLastCommand.equals(""))) {
      sendWearCommand(wearLastCommand);
    }
  }

  // Informs the application about the wear sleep state
  public void setWearDeviceAlive(boolean state) {
    wearDeviceAlive=state;
    if (wearDeviceAlive) {
      coreObject.executeCoreCommand("remoteMapInit()");
    }
  }

  // Called if a channel to a wear device is opened
  @Override
  public void onChannelOpened(Channel channel) {
  }

  // Called if a channel to a wear device is closed
  @Override
  public void onChannelClosed(Channel channel, int closeReason, int appSpecificErrorCode) {

  }

  // Called if an input from a wear device is closed
  @Override
  public void onInputClosed(Channel channel, int closeReason, int appSpecificErrorCode) {

  }

  // Called if an output from a wear device is closed
  @Override
  public void onOutputClosed(Channel channel, int closeReason, int appSpecificErrorCode) {
    if (attachment!=null) {
      channel.close(coreObject.googleApiClient);
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "channel has been closed");
      synchronized (attachment) {
        attachmentSent=true;
        attachment.notifyAll();
      }
    }
  }
}
