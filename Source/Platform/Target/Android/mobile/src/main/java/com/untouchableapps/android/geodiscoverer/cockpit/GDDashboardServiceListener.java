//============================================================================
// Name        : GDDashboardServiceListener.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================
package com.untouchableapps.android.geodiscoverer.cockpit;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.net.InetAddress;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceListener;

/** Handles communication with the GeoDashboard app in the locat network */
public class GDDashboardServiceListener implements ServiceListener {

  /** jmDNS object */
  protected JmDNS jmDNS = null;
  
  /** Core object */
  protected GDCore coreObject = null;

  /** Constructor */
  GDDashboardServiceListener(JmDNS jmDNS) {
    this.jmDNS = jmDNS;
    coreObject = GDApplication.coreObject;
  }
  
  /** Called when service has been found */
  @Override
  public void serviceAdded(ServiceEvent serviceEvent) {
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("service %s found",serviceEvent.getName()));
    if (!coreObject.coreInitialized)
      return;
    jmDNS.requestServiceInfo(serviceEvent.getType(), serviceEvent.getName(), true);
  }

  /** Called when a service has been removed */
  @Override
  public void serviceRemoved(ServiceEvent serviceEvent) {
  }

  /** Called when a service has been resolved */
  @Override
  public void serviceResolved(ServiceEvent serviceEvent) {
    if (!coreObject.coreInitialized)
      return;
    InetAddress[] host = serviceEvent.getInfo().getInetAddresses();
    coreObject.executeCoreCommand(String.format("addDashboardDevice(%s,%d)",host[0].getHostAddress(),serviceEvent.getInfo().getPort()));
  }  

}
