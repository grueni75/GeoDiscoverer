//============================================================================
// Name        : GDTools.java
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

package com.untouchableapps.android.geodiscoverer.core;

import com.google.android.gms.wearable.Node;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.FileChannel;
import java.util.Set;

public class GDTools {

  /** Geodiscoverer capability for search for wear nodes */
  public final static String WEAR_CAPABILITY_NAME = "geodiscoverer";

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

  /** Picks the best wear node to send the message to */
  public static String pickBestWearNodeId(Set<Node> nodes) {
    String bestNodeId = null;
    for (Node node : nodes) {
      if (node.isNearby()) {
        return node.getId();
      }
      bestNodeId = node.getId();
    }
    return bestNodeId;
  }

}
