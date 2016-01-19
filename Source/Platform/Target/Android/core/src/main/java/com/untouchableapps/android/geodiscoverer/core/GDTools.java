//============================================================================
// Name        : GDTools.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer.core;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.FileChannel;

public class GDTools {

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

}
