//============================================================================
// Name        : GDDashboardServiceListener.java
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
