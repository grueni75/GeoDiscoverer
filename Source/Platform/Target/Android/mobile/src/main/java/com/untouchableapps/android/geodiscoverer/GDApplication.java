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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import org.acra.ACRA;
import org.acra.ACRAConstants;
import org.acra.ReportField;
import org.acra.ReportingInteractionMode;
import org.acra.annotation.ReportsCrashes;
import org.acra.sender.GoogleFormSender;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Environment;
import android.os.Message;
import android.support.design.widget.Snackbar;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.wearable.MessageApi;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.NodeApi;
import com.google.android.gms.wearable.Wearable;
import com.untouchableapps.android.geodiscoverer.cockpit.CockpitEngine;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

/* Configuration of ACRA for reporting crashes */
@ReportsCrashes(
    formKey = "", 
    formUri = "https://grueni75.cloudant.com/acra-geodiscoverer/_design/acra-storage/_update/report", 
    reportType = org.acra.sender.HttpSender.Type.JSON, 
    httpMethod = org.acra.sender.HttpSender.Method.PUT, 
    mode = ReportingInteractionMode.NOTIFICATION,
    resNotifTickerText = R.string.crash_notification_ticker_text,
    resNotifTitle = R.string.crash_notification_title,
    resNotifText = R.string.crash_notification_text,
    resDialogTitle = R.string.crash_dialog_title, 
    resDialogText = R.string.crash_dialog_text, 
    resDialogCommentPrompt = R.string.crash_dialog_comment_text, 
    resDialogOkToast = R.string.crash_dialog_ok_toast_text, 
    resDialogEmailPrompt = R.string.crash_dialog_email_text,
    resToastText = R.string.crash_toast_text_new_report, 
    formUriBasicAuthLogin = "tlyiessideratterandiffor", 
    formUriBasicAuthPassword = "rhMR0V0AmdHU7If4jk2N1OIq"
)

/* Main application class */
public class GDApplication extends Application implements GDAppInterface, GoogleApiClient.ConnectionCallbacks {

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

  /** Reference to the wear message api */
  GoogleApiClient googleApiClient = null;

  /** Time to wait for a connection */
  final static long WEAR_CONNECTION_TIME_OUT_MS = 1000;

  /** List of commands to send to wear */
  LinkedBlockingQueue<String> wearCommands = new LinkedBlockingQueue<String>();

  /** Thread that sends commands */
  Thread wearThread = null;

  /** Called when the application starts */
  @Override
  public void onCreate() {
    super.onCreate();  

    // Init crash reporting
    ACRA.init(this);
        
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
            ACRA.getConfig().setResToastText(R.string.crash_toast_text_left_over_report);
            sendNativeCrashReport(file.getAbsolutePath(), true);
            break; // only one report at a time
          }
        }
      }
    }

    // Init the message api to communicate with wear device
    googleApiClient = new GoogleApiClient.Builder(this).addApi(Wearable.API).build();
    googleApiClient.connect();

    // Start the thread that handles the wear communication
    wearThread = new Thread(new Runnable() {
      @Override
      public void run() {
        while (true) {
          try {
            String command = wearCommands.take();
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","processing wear command: " + command);
            NodeApi.GetConnectedNodesResult nodes =
                Wearable.NodeApi.getConnectedNodes(googleApiClient).await(WEAR_CONNECTION_TIME_OUT_MS,
                    TimeUnit.MILLISECONDS);
            if (nodes != null) {
              for (Node node : nodes.getNodes()) {
                MessageApi.SendMessageResult result = Wearable.MessageApi.sendMessage(
                    googleApiClient, node.getId(), "/com.untouchableapps.android.geodiscoverer",
                    command.getBytes()).await(WEAR_CONNECTION_TIME_OUT_MS, TimeUnit.MILLISECONDS);
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
      String[] contentsInLines = contents.split("\n");
      ReportField[] customReportFields = new ReportField[ACRAConstants.DEFAULT_REPORT_FIELDS.length+1];
      System.arraycopy(ACRAConstants.DEFAULT_REPORT_FIELDS, 0, customReportFields, 0, ACRAConstants.DEFAULT_REPORT_FIELDS.length);
      customReportFields[ACRAConstants.DEFAULT_REPORT_FIELDS.length]=ReportField.APPLICATION_LOG;
      ACRA.getConfig().setCustomReportContent(customReportFields);
      ACRA.getConfig().setApplicationLogFileLines(contentsInLines.length+1);
      ACRA.getConfig().setApplicationLogFile(txtPath);
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

}
