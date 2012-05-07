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

package com.perfectapp.android.geodiscoverer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;

import android.app.Application;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.Toast;

/* Main application class */
public class GDApplication extends Application {

  /** Interface to the native C++ core */
  public static GDCore coreObject=null;
  
  /** Message log */
  public static String messages = "";
  
  // Severity levels for messages
  public static final int ERROR_MSG = 0;
  public static final int WARNING_MSG = 1;
  public static final int INFO_MSG = 2;
  public static final int DEBUG_MSG = 3;
  public static final int FATAL_MSG = 4;
    
  /** Called when the application starts */
  @Override
  public void onCreate() {
    super.onCreate();  

    // Initialize the core object
    String homeDirPath = GDApplication.getHomeDirPath();
    if (homeDirPath.equals("")) {
      Toast.makeText(this, String.format(String.format(getString(R.string.no_home_dir),homeDirPath)), Toast.LENGTH_LONG).show();
    } else {
      coreObject=new GDCore(homeDirPath);
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
    if (coreObject!=null) {
      coreObject.executeAppCommand("updateMessages()");
    }
  }
}
