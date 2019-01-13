//============================================================================
// Name        : GDMessageListenerService.java
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

import android.net.Uri;
import android.os.Bundle;
import android.os.Message;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.wearable.Channel;
import com.google.android.gms.wearable.ChannelApi;
import com.google.android.gms.wearable.ChannelClient;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Wearable;
import com.google.android.gms.wearable.WearableListenerService;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.HashMap;
import java.util.Hashtable;

public class GDMessageListenerService extends WearableListenerService {

  /** Called when a channel is opened */
  @Override
  public void onChannelOpened(Channel channel) {
    super.onChannelOpened(channel);
    GDCore coreObject = ((GDApplication) getApplication()).coreObject;
    if (coreObject == null)
      return;
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "channel opened: " + channel.getPath());
    if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/mapArchive/")) {
      String path=coreObject.executeCoreCommand("getFreeMapArchiveFilePath()");
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "creating map archive <" + path + ">");
      File f;
      try {
        f = new File(path);
        f.createNewFile();
      }
      catch (IOException e) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
        return;
      }
      coreObject.channelPathToFilePath.put(new String(channel.getPath()), path);
      channel.receiveFile(coreObject.googleApiClient, Uri.fromFile(f), false);
      //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "setRemoteServerActive(1)");
      coreObject.executeCoreCommand("setRemoteServerActive(1)");
    }
  }

  /** Called when a channel is closed */
  @Override
  public void onChannelClosed(Channel channel, int closeReason, int appSpecificErrorCode) {
    super.onChannelClosed(channel, closeReason, appSpecificErrorCode);
    GDCore coreObject = ((GDApplication) getApplication()).coreObject;
    if (coreObject == null)
      return;
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "channel closed: " + channel.getPath());
    if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/mapArchive/")) {
      //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "close reason: " + closeReason);
      String path = (String) coreObject.channelPathToFilePath.get(channel.getPath());
      if (path != null) {
        if (closeReason == ChannelApi.ChannelListener.CLOSE_REASON_REMOTE_CLOSE) {
          coreObject.executeCoreCommand("addMapArchive(" + path + ")");
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "map archive <" + path + "> created");
        } else {
          File f = new File(path);
          f.delete();
        }
        coreObject.channelPathToFilePath.remove(channel.getPath());
        if (coreObject.channelPathToFilePath.size()==0) {
          //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "setRemoteServerActive(0)");
          coreObject.executeCoreCommand("setRemoteServerActive(0)");
        }
      }
    }
  }

  /** Called when a message is received */
  @Override
  public void onMessageReceived( final MessageEvent messageEvent ) {
    super.onMessageReceived(messageEvent);
    /*GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("message received: %s %s",
            messageEvent.getPath(),
            new String(messageEvent.getData()))
    );*/
    GDCore coreObject = ((GDApplication) getApplication()).coreObject;
    if (coreObject==null)
      return;
    if (!coreObject.coreInitialized)
      return;
    if (messageEvent.getPath().equals("/com.untouchableapps.android.geodiscoverer")) {
      String cmd = new String(messageEvent.getData());
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", cmd.substring(0,cmd.indexOf(")")+1));
      if (cmd.equals("forceRemoteMapUpdate()")) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","map update requested by remote server");
        coreObject.executeCoreCommand("forceMapUpdate()");
      }
      if (cmd.startsWith("setAllNavigationInfo")) {
        String args1 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeAppCommand("setFormattedNavigationInfo" + args1);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args2 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommand("setPlainNavigationInfo" + args2);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args3 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommand("setLocationPos" + args3);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args4 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommand("setTargetPos" + args4);
        ((GDApplication) getApplication()).executeAppCommand("updateScreen()");
      }
      if (cmd.equals("getWearDeviceAlive()")) {
        coreObject.executeAppCommand("setWearDeviceAlive(1)");
      }
      if (cmd.startsWith("setNewRemoteMap")) {

        // Set the new config
        boolean configChanged=false;
        while (cmd.length()>0) {
          String args = cmd.substring(cmd.indexOf("("), cmd.indexOf(")") + 1);
          String path = args.substring(1,args.indexOf(","));
          args = args.substring(args.indexOf(",")+1);
          String name = args.substring(0,args.indexOf(","));
          String value = args.substring(args.indexOf(",")+1,args.indexOf(")"));
          String oldValue = coreObject.configStoreGetStringValue(path,name);
          Bundle info = coreObject.configStoreGetNodeInfo(path + "/" + name);
          if (info.getString("type").equals("integer")) {
            int t1 = Integer.valueOf(value);
            int t2 = Integer.valueOf(oldValue);
            if (t1!=t2) {
              coreObject.configStoreSetStringValue(path, name, value);
              configChanged = true;
            }
          }
          if (info.getString("type").equals("double")) {
            double t1 = Double.valueOf(value);
            double t2 = Double.valueOf(oldValue);
            if (t1!=t2) {
              coreObject.configStoreSetStringValue(path, name, value);
              configChanged=true;
            }
          }
          if (info.getString("type").equals("string")) {
            if (!value.equals(oldValue)) {
              coreObject.configStoreSetStringValue(path, name, value);
              configChanged=true;
            }
          }
          cmd = cmd.substring(cmd.indexOf(")") + 1);
        }

        // Restart the core
        if (configChanged) {
          coreObject.configStoreSetStringValue("Map/Remote", "reset", "1");
          Message m = Message.obtain(coreObject.messageHandler);
          m.what = GDCore.RESTART_CORE;
          Bundle b = new Bundle();
          b.putBoolean("resetConfig", false);
          m.setData(b);
          coreObject.messageHandler.sendMessage(m);
        } else {
          coreObject.executeCoreCommand("forceMapUpdate()");
        }
      }
    }
  }
}
