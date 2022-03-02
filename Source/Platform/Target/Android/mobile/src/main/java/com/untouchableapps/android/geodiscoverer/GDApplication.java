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

import android.Manifest;
import android.app.Activity;
import android.app.AlarmManager;
import android.app.Application;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Base64;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.ChannelClient;
import com.google.android.gms.wearable.Wearable;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.GDTools;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitInfos;
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask;
import com.untouchableapps.android.geodiscoverer.logic.GDService;
import com.untouchableapps.android.geodiscoverer.logic.cockpit.CockpitEngine;
import com.untouchableapps.android.geodiscoverer.ui.activity.AuthenticateGoogleBookmarks;
import com.untouchableapps.android.geodiscoverer.ui.activity.RequestPermissions;

import org.acra.ACRA;
import org.acra.config.CoreConfigurationBuilder;
import org.acra.config.MailSenderConfigurationBuilder;
import org.acra.config.NotificationConfigurationBuilder;

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
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import androidx.annotation.NonNull;

/* Main application class */
public class GDApplication extends Application implements GDAppInterface {

  // Notification IDs
  static public final int NOTIFICATION_STATUS_ID=1;
  static public final int NOTIFICATION_ACCESSIBILITY_SERVICE_NOT_ENABLED_ID=2;

  /** Required permissions */
  public static final String[] requiredPermissions = {
      Manifest.permission.READ_EXTERNAL_STORAGE,
      Manifest.permission.WRITE_EXTERNAL_STORAGE,
      Manifest.permission.ACCESS_COARSE_LOCATION,
      Manifest.permission.ACCESS_FINE_LOCATION,
      Manifest.permission.WAKE_LOCK,
      Manifest.permission.INTERNET,
      Manifest.permission.VIBRATE,
      Manifest.permission.BLUETOOTH,
      Manifest.permission.BLUETOOTH_ADMIN
  };

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;

  /** Background tasks */
  public static GDBackgroundTask backgroundTask=new GDBackgroundTask();

  /** Message log */
  public static String messages = "";
  
  /** Maximum size of the message log */
  final static int maxMessagesSize = 4096;

  /** Dashboard network service type */
  public static final String dashboardNetworkServiceType = "_geodashboard._tcp.local.";

  /** Cockpit engine */
  CockpitEngine cockpitEngine = null;

  /** Reference to the viewmap activity */
  public Activity activity = null;
  public Handler messageHandler = null;

  /** Time to wait for a connection */
  final static long WEAR_CONNECTION_TIME_OUT_MS = 1000;

  /** List of commands to send to wear */
  LinkedBlockingQueue<String> wearMessageCommands = new LinkedBlockingQueue<String>();

  /** List of files to send to wear */
  LinkedBlockingQueue<String> wearFileCommands = new LinkedBlockingQueue<String>();

  /** Threads that handles wear messages */
  Thread wearMessageThread = null;

  /** Threads that handles wear file transfers */
  Thread wearFileThread = null;

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

    // Init ACRA
    CoreConfigurationBuilder builder = new CoreConfigurationBuilder(this);
    builder
        .withBuildConfigClass(BuildConfig.class);
    builder.getPluginConfigurationBuilder(NotificationConfigurationBuilder.class)
        .withResTickerText(R.string.crash_notification_ticker_text)
        .withResTitle(R.string.crash_notification_title)
        .withResText(R.string.crash_notification_text)
        .withResChannelName(R.string.crash_notification_channel)
        .withEnabled(true);
    builder.getPluginConfigurationBuilder(MailSenderConfigurationBuilder.class)
        .withMailTo("matthias.gruenewald@gmail.com")
        .withResSubject(R.string.crash_mail_subject)
        .withEnabled(true);
    ACRA.init(this, builder);
  }

  /** Called when the application starts */
  @Override
  public void onCreate() {

    /*StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
        .detectAll()
        .penaltyLog()
        .build());
    StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
        .detectAll()
        .penaltyLog()
        .build());*/
    super.onCreate();  

    // Initialize the core object
    String homeDirPath = GDCore.getHomeDirPath();
    if (homeDirPath.equals("")) {
      Toast.makeText(this, String.format(String.format(getString(R.string.no_home_dir),homeDirPath)), Toast.LENGTH_LONG).show();
      System.exit(1);
    } else {

      // Check if we have the required permissions
      if (!checkPermissions(this)) {

        // Start the activity to request the permission
        Intent intent = new Intent(this, RequestPermissions.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        return;
      }
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
            sendNativeCrashReport(file.getAbsolutePath(), false);
            break; // only one report at a time
          }
        }
      }
    }

    // Start the thread that handles the wear messages
    wearMessageThread = new Thread(new Runnable() {
      @Override
      public void run() {
        coreObject.setThreadPriority(2);
        long lastNavigationInfoUpdate=0;
        boolean prevSendMessageForced=false;
        while (true) {
          try {

            // Get command
            String command = wearMessageCommands.take();
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","processing wear message command: " + command);

            // Only process command if device is alive
            if ((wearDeviceAlive)||(command.equals("getWearDeviceAlive()"))) {

              // Check if it is necessary to send command
              long t=System.currentTimeMillis();
              boolean sendMessage = true;

              // Handle navigation infos
              if (command.startsWith("setAllNavigationInfo")) {

                // If wear device is inactive, check if message needs to be sent
                if (!wearDeviceSleeping) {
                  lastNavigationInfoUpdate=t;
                } else {

                  // Always send message if off route or target approaching
                  sendMessage=false;
                  String infosAsString = command.substring(command.indexOf("("), command.indexOf(")") + 1);
                  CockpitInfos infos = CockpitEngine.createCockpitInfo(infosAsString);
                  if ((!infos.turnDistance.equals("-")) || (infos.offRoute)) {
                    sendMessage = true;
                    prevSendMessageForced = true;
                  } else {

                    // Send the next message also after important event is over such that watch face shows correct contents
                    if (prevSendMessageForced)
                      sendMessage = true;
                    else {

                      // Always update the watch face every minute
                      if ((t-lastNavigationInfoUpdate)>60*1000) {
                        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","setAllNavigationInfo outdated");
                        sendMessage = true;
                      }
                    }
                    prevSendMessageForced = false;
                  }
                  if (sendMessage) {
                    lastNavigationInfoUpdate = t;
                  }
                }
              }
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "sendMessage=" + String.valueOf(sendMessage));

              // Send command
              if (sendMessage) {
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
                //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "command=" + command);
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
          catch (ExecutionException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
          catch (TimeoutException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","watch not reachable");
          }
        }
      }
    });
    wearMessageThread.start();

    // Install a callback when the output is closed (file is received at other side)
    Wearable.getChannelClient(getApplicationContext()).registerChannelCallback(new ChannelClient.ChannelCallback() {
      @Override
      public void onOutputClosed(@NonNull ChannelClient.Channel channel, int i, int i1) {
        super.onOutputClosed(channel, i, i1);
        if (attachment!=null) {
          Wearable.getChannelClient(getApplicationContext()).close(channel);
          //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "channel has been closed");
          synchronized (attachment) {
            attachmentSent=true;
            attachment.notifyAll();
          }
        }
      }
    });

    // Start the thread that handles the wear file transfers
    wearFileThread = new Thread(new Runnable() {
      @Override
      public void run() {
        coreObject.setThreadPriority(2);
        long lastUpdate=0;
        boolean prevSendMessageForced=false;
        while (true) {
          try {

            // Get command
            String command = wearFileCommands.take();
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","processing wear file command: " + command);

            // Get the file to sent
            String postfix="unknown";
            String acknowledgeCmd="";
            String acknowledgeID="";
            String path="";
            if (command.startsWith("serveRemoteMapArchive")) {
              path = command.substring(command.indexOf("(")+1, command.indexOf(","));
              attachment = new File(path);
              postfix = "mapArchive";
              command=command.substring(command.indexOf(",")+1);
              String id = command.substring(0, command.indexOf(","));
              acknowledgeCmd="remoteMapArchiveServed";
              acknowledgeID=id;
            }
            if (command.startsWith("serveRemoteOverlayArchive")) {
              path = command.substring(command.indexOf("(")+1, command.indexOf(","));
              attachment = new File(path);
              postfix = "overlayArchive";
              command=command.substring(command.indexOf(",")+1);
              String id = command.substring(0, command.indexOf(","));
              acknowledgeCmd="remoteOverlayArchiveServed";
              acknowledgeID=id;
            }
            String hash = command.substring(command.indexOf(",")+1, command.indexOf(")"));
            postfix = postfix + "/" + hash;
            path = path.substring(path.indexOf("/Map"));

            // Send file
            CapabilityInfo capabilityInfo = Tasks.await(
                Wearable.getCapabilityClient(getApplicationContext()).getCapability(
                    GDTools.WEAR_CAPABILITY_NAME, CapabilityClient.FILTER_REACHABLE),
                GDTools.wearTransmissionTimout, TimeUnit.MILLISECONDS);
            String nodeId = GDTools.pickBestWearNodeId(capabilityInfo.getNodes());
            //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "found node id: " + nodeId);
            attachmentSent=false;
            if (nodeId!=null) {
              Task<ChannelClient.Channel> openChannelTask=Wearable.getChannelClient(getApplicationContext()).openChannel(nodeId,"/com.untouchableapps.android.geodiscoverer/" + postfix + path);
              openChannelTask.addOnCompleteListener(new OnCompleteListener<ChannelClient.Channel>() {
                @Override
                public void onComplete(@NonNull Task<ChannelClient.Channel> task) {
                  //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "channel has been opened");
                  Wearable.getChannelClient(getApplicationContext()).sendFile(task.getResult(), Uri.fromFile(attachment));
                }
              });
            }
            synchronized (attachment) {
              while (!attachmentSent)
                attachment.wait();
            }
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "file " + attachment.getAbsolutePath() + " sent to remote device");
            synchronized (attachment) {
              attachment=null;
            }

            // Inform core that transfer is over after some delay
            final String delayedAcknowledgeID = acknowledgeID;
            final String delayedAcknowledgeCmd = acknowledgeCmd;
            new Timer().schedule(new TimerTask() {
              @Override
              public void run() {
                coreObject.executeCoreCommand(delayedAcknowledgeCmd,delayedAcknowledgeID);
              }
            }, 1000);
          }
          catch (InterruptedException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
          catch (ExecutionException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
          catch (TimeoutException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","watch not reachable");
          }
        }
      }
    });
    wearFileThread.start();

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
        coreObject.executeCoreCommand("log", severityString, tag, message);
      }
      coreObject.executeAppCommand("updateMessages()");
    }
  }

  public static final long MESSAGE_BAR_DURATION_SHORT = 2000;
  public static final long MESSAGE_BAR_DURATION_LONG = 4000;

  /** Sets the view map activity */
  public void setActivity(Activity activity, Handler messageHandler) {
    this.activity = activity;
    this.messageHandler = messageHandler;
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
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", logPath);
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

  // Schedules a restart of the GDService
  @Override
  public void scheduleRestart() {
    Intent intent = new Intent(getApplicationContext(), GDService.class);
    intent.setAction("scheduledRestart");
    PendingIntent pendingIntent = PendingIntent.getService(getApplicationContext(),0,intent,0);
    AlarmManager alarmManager = (AlarmManager)getApplicationContext().getSystemService(Context.ALARM_SERVICE);
    alarmManager.set(AlarmManager.RTC, System.currentTimeMillis() + 10000, pendingIntent);
  }

  // Returns the application context
  @Override
  public Context getContext() {
    return getApplicationContext();
  }

  // Sends a command to the activity
  @Override
  public void executeAppCommand(String cmd) {
    if (cmd.equals("coreInitialized()")) {

      // Inform the service
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("coreInitialized");
      startService(intent);
    }
    if (cmd.equals("earlyInitComplete()")) {

      // Inform the service
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("earlyInitComplete");
      startService(intent);

      // Init the background task
      backgroundTask.onCreate(coreObject);
    }
    if (cmd.equals("lateInitComplete()")) {

      // Inform the service
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("lateInitComplete");
      startService(intent);

      // Ask wear device for current power status
      sendWearCommand("getWearDeviceAlive()");
    }
    if (cmd.equals("authenticateGoogleBookmarks()")) {
      Intent intent = new Intent(this, AuthenticateGoogleBookmarks.class);
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      startActivity(intent);
      return;
    }
    if (messageHandler != null) {
      Message m = Message.obtain(messageHandler);
      m.what = 0;
      Bundle b = new Bundle();
      b.putString("command", cmd);
      m.setData(b);
      messageHandler.sendMessage(m);
    } else {
      if (cmd.equals("exitActivity()")) {
        System.exit(0);
      }
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
    if ((command.startsWith("serveRemoteMapArchive"))||(command.startsWith("serveRemoteOverlayArchive"))) {
      wearFileCommands.offer(command);
    } else {
      wearMessageCommands.offer(command);
    }
  }

  // Informs the application about the wear sleep state
  public void setWearDeviceSleeping(boolean state) {
    if (!coreObject.coreInitialized)
      return;
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","wear device sleeping state is "+state);
    if (wearDeviceSleeping&&!state) {
      sendWearCommand("setRemoteBattery(" + coreObject.batteryStatus + ")");
    }
    wearDeviceSleeping=state;
    if ((wearDeviceAlive)&&(!wearLastCommand.equals(""))) {
      sendWearCommand(wearLastCommand);
    }
  }

  // Informs the application about the wear sleep state
  public void setWearDeviceAlive(boolean state) {
    if (!coreObject.coreInitialized)
      return;
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","wear device alive state is "+state);
    wearDeviceAlive=state;
    if (wearDeviceAlive) {
      coreObject.executeCoreCommand("remoteMapInit");
      sendWearCommand("setRemoteBattery(" + coreObject.batteryStatus + ")");
    } else {
      wearDeviceSleeping=true;
    }
  }

  // Checks if all required permissions are available
  public static boolean checkPermissions(Context context) {
    for (int i = 0; i < requiredPermissions.length; i++) {
      if(context.checkSelfPermission(requiredPermissions[i])!= PackageManager.PERMISSION_GRANTED) {
        return false;
      }
    }
    if (!Settings.canDrawOverlays(context)) {
      return false;
    }
    return true;
  }


}
