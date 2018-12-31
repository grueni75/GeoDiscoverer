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

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Wearable;
import com.google.android.gms.wearable.WearableListenerService;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;

import java.util.HashMap;

public class GDMessageListenerService extends WearableListenerService {

  /** Called when a message is received */
  @Override
  public void onMessageReceived( final MessageEvent messageEvent ) {
    super.onMessageReceived(messageEvent);
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("message received: %s %s",
            messageEvent.getPath(),
            new String(messageEvent.getData()))
    );
    if (messageEvent.getPath().equals("/com.untouchableapps.android.geodiscoverer")) {
      String cmd = new String(messageEvent.getData());
      if (cmd.startsWith("setWearDeviceSleeping(1)")) {
        ((GDApplication)getApplication()).setWearDeviceSleeping(true);
      }
      if (cmd.startsWith("setWearDeviceSleeping(0)")) {
        ((GDApplication)getApplication()).setWearDeviceSleeping(false);
      }
      if (cmd.startsWith("setWearDeviceAlive(1)")) {
        ((GDApplication) getApplication()).setWearDeviceAlive(true);
      }
      if (cmd.startsWith("setWearDeviceAlive(0)")) {
        ((GDApplication)getApplication()).setWearDeviceAlive(false);
      }
      if (cmd.startsWith("findRemoteMapTileByGeographicCoordinate(")) {
        ((GDApplication)getApplication()).coreObject.executeCoreCommand(cmd);
      }
      if (cmd.startsWith("fillGeographicAreaWithRemoteTiles(")) {
        ((GDApplication)getApplication()).coreObject.executeCoreCommand(cmd);
      }
    }
  }
}
