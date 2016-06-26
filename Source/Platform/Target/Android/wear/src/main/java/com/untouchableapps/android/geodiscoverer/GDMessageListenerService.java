//============================================================================
// Name        : GDMessageListenerService.java
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

import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.WearableListenerService;

public class GDMessageListenerService extends WearableListenerService{

  /** Called when a message is received */
  @Override
  public void onMessageReceived( final MessageEvent messageEvent ) {
    /*GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("message received: %s %s",
            messageEvent.getPath(),
            new String(messageEvent.getData()))
    );*/
    if (messageEvent.getPath().equals("/com.untouchableapps.android.geodiscoverer")) {
      String cmd = new String(messageEvent.getData());
      if (cmd.startsWith("setFormattedNavigationInfo(")) {
        ((GDApplication) getApplication()).coreObject.executeAppCommand(cmd);
      } else {
        if (GDApplication.coreObject != null) {
          GDApplication.coreObject.executeCoreCommand(cmd);
          if (cmd.startsWith("setPlainNavigationInfo("))
            ((GDApplication) getApplication()).executeAppCommand("updateScreen()");
        }
      }
    }
  }

}
