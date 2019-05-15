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
    String path = "unknown";
    String hash = "unknown";
    if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/mapArchive/")) {
      hash=channel.getPath().substring(54);
    }
    if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/overlayArchive/")) {
      hash=channel.getPath().substring(58);
    }
    path=coreObject.homePath + hash.substring(hash.indexOf("/"));
    hash=hash.substring(0,hash.indexOf("/"));
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "receiving file <" + path + "> with hash <" + hash + ">");
    File f;
    try {
      f = new File(path);
      f.createNewFile();
    }
    catch (IOException e) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
      return;
    }
    Bundle params = new Bundle();
    params.putString("path",path);
    params.putString("hash",hash);
    coreObject.channelPathToFilePath.put(new String(channel.getPath()), params);
    channel.receiveFile(coreObject.googleApiClient, Uri.fromFile(f), false);
    coreObject.executeCoreCommand("setRemoteServerActive","1");
  }

  /** Called when a channel is closed */
  @Override
  public void onChannelClosed(Channel channel, int closeReason, int appSpecificErrorCode) {
    super.onChannelClosed(channel, closeReason, appSpecificErrorCode);
    GDCore coreObject = ((GDApplication) getApplication()).coreObject;
    if (coreObject == null)
      return;
    Bundle params = (Bundle) coreObject.channelPathToFilePath.get(channel.getPath());
    if (params != null) {
      if (closeReason == ChannelApi.ChannelListener.CLOSE_REASON_REMOTE_CLOSE) {
        if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/mapArchive/")) {
          coreObject.executeCoreCommand("addMapArchive",params.getString("path"),params.getString("hash"));
        }
        if (channel.getPath().startsWith("/com.untouchableapps.android.geodiscoverer/overlayArchive/")) {
          coreObject.executeCoreCommand("addOverlayArchive",params.getString("path"), params.getString("hash"));
        }
      } else {
        File f = new File(params.getString("path"));
        f.delete();
      }
      coreObject.channelPathToFilePath.remove(channel.getPath());
      if (coreObject.channelPathToFilePath.size()==0) {
        coreObject.executeCoreCommand("setRemoteServerActive","0");
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
      boolean cmdExecuted=false;
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", cmd.substring(0,cmd.indexOf(")")+1));
      if (cmd.equals("forceRemoteMapUpdate()")) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp","map update requested by remote server");
        coreObject.executeCoreCommand("forceMapUpdate");
        cmdExecuted=true;
      }
      if (cmd.startsWith("setAllNavigationInfo")) {
        String args1 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeAppCommand("setFormattedNavigationInfo" + args1);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args2 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommandRaw("setPlainNavigationInfo" + args2);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args3 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommandRaw("setLocationPos" + args3);
        cmd = cmd.substring(cmd.indexOf(")") + 1);
        String args4 = cmd.substring(cmd.indexOf("("), cmd.indexOf(")")+1);
        coreObject.executeCoreCommandRaw("setTargetPos" + args4);
        ((GDApplication) getApplication()).executeAppCommand("updateScreen()");
        cmdExecuted=true;
      }
      if (cmd.equals("getWearDeviceAlive()")) {
        coreObject.executeAppCommand("setWearDeviceAlive(1)");
        cmdExecuted=true;
      }
      if (cmd.startsWith("setNewRemoteMap")) {

        // Set the new config
        boolean configChanged=false;
        while (cmd.length()>0) {
          String args = cmd.substring(cmd.indexOf("("), cmd.indexOf(")") + 1);
          //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "args=" + args);
          String path = args.substring(1,args.indexOf(","));
          args = args.substring(args.indexOf(",")+1);
          String name = args.substring(0,args.indexOf(","));
          args = args.substring(args.indexOf(",")+1);
          String value = args.substring(0,args.indexOf(","));
          boolean testForConfigChange = Integer.valueOf(args.substring(args.indexOf(",")+1,args.indexOf(")"))) > 0 ? true : false;
          //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "path=" + path + " name=" + name + " value=" + value + " testForConfigChange=" + Boolean.toString(testForConfigChange));
          String oldValue = coreObject.configStoreGetStringValue(path,name);
          Bundle info = coreObject.configStoreGetNodeInfo(path + "/" + name);
          if (info.getString("type").equals("integer")) {
            int t1 = Integer.valueOf(value);
            int t2 = Integer.valueOf(oldValue);
            if (t1!=t2) {
              coreObject.configStoreSetStringValue(path, name, value);
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "name=" + name + " value=" + value);
              if (testForConfigChange)
                configChanged = true;
            }
          }
          if (info.getString("type").equals("double")) {
            double t1 = Double.valueOf(value);
            double t2 = Double.valueOf(oldValue);
            if (t1!=t2) {
              coreObject.configStoreSetStringValue(path, name, value);
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "name=" + name + " value=" + value);
              if (testForConfigChange)
                configChanged=true;
            }
          }
          if (info.getString("type").equals("string")) {
            if (!value.equals(oldValue)) {
              coreObject.configStoreSetStringValue(path, name, value);
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "name=" + name + " value=" + value);
              if (testForConfigChange)
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
          coreObject.executeCoreCommand("forceMapUpdate");
        }
        cmdExecuted=true;
      }

      // Forward all other commands
      if (!cmdExecuted) {
        coreObject.executeCoreCommandRaw(cmd);
      }
    }
  }
}
