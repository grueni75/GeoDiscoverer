//============================================================================
// Name        : GDApplication.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.FileChannel;

import org.acra.ACRA;
import org.acra.ReportingInteractionMode;
import org.acra.annotation.ReportsCrashes;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import com.cocosw.undobar.UndoBarController;

/* Configuration of ACRA for reporting crashes */
@ReportsCrashes(
    formKey = "", 
    formUri = "https://grueni75.cloudant.com/acra-geodiscoverer/_design/acra-storage/_update/report", 
    reportType = org.acra.sender.HttpSender.Type.JSON, 
    httpMethod = org.acra.sender.HttpSender.Method.PUT, 
    mode = ReportingInteractionMode.DIALOG, 
    resDialogTitle = R.string.crash_dialog_title, 
    resDialogText = R.string.crash_dialog_text, 
    resDialogCommentPrompt = R.string.crash_dialog_comment_text, 
    resDialogOkToast = R.string.crash_dialog_ok_toast_text, 
    resDialogEmailPrompt = R.string.crash_dialog_email_text,
    resToastText = R.string.crash_toast_text, 
    formUriBasicAuthLogin = "tlyiessideratterandiffor", 
    formUriBasicAuthPassword = "rhMR0V0AmdHU7If4jk2N1OIq"
)

/* Main application class */
public class GDApplication extends Application {

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;
  
  /** Message log */
  public static String messages = "";
  
  /** Maximum size of the message log */
  final static int maxMessagesSize = 4096;
  
  // Severity levels for messages
  public static final int ERROR_MSG = 0;
  public static final int WARNING_MSG = 1;
  public static final int INFO_MSG = 2;
  public static final int DEBUG_MSG = 3;
  public static final int FATAL_MSG = 4;
  
  /** Application context */
  public static Context appContext;
  
  /** Called when the application starts */
  @Override
  public void onCreate() {
    super.onCreate();  
    
    // Get application context
    appContext = getApplicationContext();
    
    // Init crash reporting
    ACRA.init(this);
    
    // Initialize the core object
    String homeDirPath = GDApplication.getHomeDirPath();
    if (homeDirPath.equals("")) {
      Toast.makeText(this, String.format(String.format(getString(R.string.no_home_dir),homeDirPath)), Toast.LENGTH_LONG).show();
      System.exit(1);
    } else {
      coreObject=new GDCore(this, homeDirPath);
    }
  }

  /** Copies a source file to a destination file */
  public static void copyFile(String srcFilename, String dstFilename) throws IOException {
    File dstFile = new File(dstFilename);
    File srcFile = new File(srcFilename);    
    if(!dstFile.exists()) {
      dstFile.createNewFile();
    }
    FileChannel source = null;
    FileChannel destination = null;
    try {
      source = new FileInputStream(srcFile).getChannel();
      destination = new FileOutputStream(dstFile).getChannel();
      destination.transferFrom(source, 0, source.size());
    }
    finally {
      if(source != null) {
        source.close();
      }
      if(destination != null) {
        destination.close();
      }
    }
  }  
  
  /** Copies a source file to a destination file */
  public static void copyFile(InputStream srcInputStream, String dstFilename) throws IOException {
    File dstFile = new File(dstFilename);    
    if(!dstFile.exists()) {
      dstFile.createNewFile();
    }
    FileChannel source = ((FileInputStream)srcInputStream).getChannel();
    FileChannel destination = null;
    try {
      destination = new FileOutputStream(dstFile).getChannel();
      destination.transferFrom(source, 0, source.size());
    }
    finally {
      if(destination != null) {
        destination.close();
      }
    }
  }  

  /** Returns the path of the home dir */
  public static String getHomeDirPath() {

    // Create the home directory if necessary
    //File homeDir=Environment.getExternalStorageDirectory(getString(R.string.home_directory));
    File externalStorageDirectory=Environment.getExternalStorageDirectory(); 
    String homeDirPath = externalStorageDirectory.getAbsolutePath() + "/GeoDiscoverer";
    File homeDir=new File(homeDirPath);
    if (!homeDir.exists()) {
      try {
        homeDir.mkdir();
      }
      catch (Exception e){
        return "";
      }
    }
    return homeDir.getAbsolutePath();
    
  }
  
  /** Adds a message to the log */
  public synchronized static void addMessage(int severityNumber, String tag, String message) {
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
    UndoBarController.UndoBar undoBar = new UndoBarController.UndoBar(activity);
    undoBar.message(message);
    undoBar.style(UndoBarController.MESSAGESTYLE);
    undoBar.duration(duration);
    undoBar.show();
  }
}
