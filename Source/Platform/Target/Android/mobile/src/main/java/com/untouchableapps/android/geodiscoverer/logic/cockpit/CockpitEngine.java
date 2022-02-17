//============================================================================
// Name        : CockpitEngine.java
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

package com.untouchableapps.android.geodiscoverer.logic.cockpit;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.net.wifi.WifiManager;

import com.untouchableapps.android.geodiscoverer.GDApplication;

import java.io.IOException;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class CockpitEngine extends com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitEngine {

  // GDApp
  GDApplication app;

  // Thread that handles network requests
  static final int NET_CMD_PLAY_SOUND_DOG = 3;
  static final int NET_CMD_PLAY_SOUND_SHIP = 4;
  static final int NET_CMD_PLAY_SOUND_CAR = 5;
  boolean quitNetworkServerThread=false;
  Thread networkServerThread=null;
  ServerSocket networkServerSocket=null;
  int networkServerPort = 11111;
  boolean networkServiceActive=false;
  void startNetworkService() {

    quitNetworkServerThread=false;
    networkServerThread = new Thread(new Runnable() {

      @Override
      public void run() {

        // Set the right priority
        coreObject.setThreadPriority(2);

        // Open server socket
        if (networkServerSocket==null) {
          try {
            networkServerSocket = new ServerSocket();
            networkServerSocket.setReuseAddress(true);
            networkServerSocket.bind(new InetSocketAddress(networkServerPort+1));
          }
          catch (IOException e) {
            GDApplication.addMessage(
                GDApplication.DEBUG_MSG,
                "GDApp",
                e.getMessage()
            );
            networkServerSocket = null;
            return;
          }
        }

        // Set bitmap to indicate "wait for first connection"
        networkServiceActive = true;

        // Handle network communication
        while (!quitNetworkServerThread) {
          try {
            Socket client = networkServerSocket.accept();
            if (quitNetworkServerThread)
              break;
            int cmd = client.getInputStream().read();
            String soundFile = "";
            switch(cmd) {
              case NET_CMD_PLAY_SOUND_SHIP:
                soundFile = "Sound/ship.mp3";
                break;
              case NET_CMD_PLAY_SOUND_CAR:
                soundFile = "Sound/car.mp3";
                break;
              case NET_CMD_PLAY_SOUND_DOG:
                soundFile = "Sound/dog.mp3";
                break;
            }
            client.close();
            app.coreObject.audioWakeup();
            final AssetFileDescriptor afd = app.getContext().getAssets().openFd(soundFile);
            final MediaPlayer player = new MediaPlayer();
            player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            player.prepare();
            player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
              public void onCompletion(MediaPlayer mp) {
                try {
                  afd.close();
                  player.release();
                } catch (IOException e) {
                  GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
                }
              }
            });
            player.start();
          }
          catch (IOException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
          }
        }
        if (networkServerSocket!=null) {
          try {
            networkServerSocket.close();
          }
          catch (IOException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",e.getMessage());
          }
        }
      }

    });
    networkServerThread.start();
  }
  void stopNetworkService() {
    quitNetworkServerThread=true;
    Thread stopThread = new Thread(new Runnable() {
      @Override
      public void run() {
        coreObject.setThreadPriority(2);
        try {
          Socket clientSocket = new Socket("localhost",networkServerSocket.getLocalPort());
          clientSocket.getOutputStream().write(255);
          clientSocket.close();
          networkServerThread.join();
        }
        catch (Exception e) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
        }
        networkServiceActive=false;
      }
    });
    stopThread.start();
    boolean repeat=true;
    while (repeat) {
      try {
        repeat=false;
        stopThread.join();
      }
      catch (InterruptedException e) {
        repeat=true;
      }
    }
  }

  // References for mDNS
  WifiManager wifiManager;
  NsdManager nsdManager;


  // References for the network discovery via mDNS
  NsdManager.DiscoveryListener dashboardServiceDiscoveryListener=null;
  String DASHBOARD_SERVICE_TYPE = "_geodashboard._tcp.";

  /** Constructor */
  public CockpitEngine(GDApplication app) {

    // Call constructor
    super(app.getContext(),GDApplication.coreObject);

    // Remember context
    this.app = app;

    // Get services
    wifiManager = (WifiManager) app.getApplicationContext().getSystemService(android.content.Context.WIFI_SERVICE);
    nsdManager = (NsdManager) app.getApplicationContext().getSystemService(Context.NSD_SERVICE);

    // Add all activated apps
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/MetaWatch", "active"))>0) {
      apps.add(new CockpitAppMetaWatch(app.getContext()));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "active"))>0) {
      apps.add(new CockpitAppVoice(app.getContext(),this));
    }
  }
  
  /** Starts the cockpit apps */
  public void start() {

    // Search for geo dashboard devices if configured
    if (Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "active"))>0) {

      // Start the service discovery
      // Instantiate a new DiscoveryListener
      dashboardServiceDiscoveryListener = new NsdManager.DiscoveryListener() {

        @Override
        public void onDiscoveryStarted(String regType) {
          app.addMessage(app.DEBUG_MSG, "GDApp", "service discovery for dashboard devices started");
        }

        @Override
        public void onServiceFound(NsdServiceInfo service) {
          // A service was found! Do something with it.
          app.addMessage(app.DEBUG_MSG, "GDApp", "service discovery success: " + service);
          if (!service.getServiceType().equals(DASHBOARD_SERVICE_TYPE)) {
            app.addMessage(app.DEBUG_MSG, "GDApp", "unknown service type: " + service.getServiceType());
          } else if (service.getServiceName().contains("GeoDashboard")){
            nsdManager.resolveService(service, new NsdManager.ResolveListener() {
              @Override
              public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
                app.addMessage(app.DEBUG_MSG, "GDApp", "service resolve failed with error code: " + errorCode);
              }

              @Override
              public void onServiceResolved(NsdServiceInfo serviceInfo) {
                app.coreObject.executeCoreCommand("addDashboardDevice",serviceInfo.getHost().getHostAddress(),String.valueOf(serviceInfo.getPort()));
              }
            });
          }
        }

        @Override
        public void onServiceLost(NsdServiceInfo service) {
          app.addMessage(app.DEBUG_MSG, "GDApp", "service lost: " + service);
        }

        @Override
        public void onDiscoveryStopped(String serviceType) {
          app.addMessage(app.DEBUG_MSG, "GDApp", "discovery stopped: " + serviceType);
        }

        @Override
        public void onStartDiscoveryFailed(String serviceType, int errorCode) {
          app.addMessage(app.DEBUG_MSG, "GDApp", "discovery start failed with error code: " + errorCode);
          nsdManager.stopServiceDiscovery(this);
        }

        @Override
        public void onStopDiscoveryFailed(String serviceType, int errorCode) {
          app.addMessage(app.DEBUG_MSG, "GDApp", "discovery start failed with error code: " + errorCode);
          nsdManager.stopServiceDiscovery(this);
        }
      };
      nsdManager.discoverServices(DASHBOARD_SERVICE_TYPE,NsdManager.PROTOCOL_DNS_SD,dashboardServiceDiscoveryListener);
    }

    // Start the network thread
    startNetworkService();

    // Do the inherited stuff
    super.start();
  }

  /** Stops the cockpit apps */
  public void stop() {

    // Stop the network service
    stopNetworkService();

    // Stop ARP cache lookup
    if (dashboardServiceDiscoveryListener!=null) {
      app.addMessage(app.DEBUG_MSG, "GDApp", "stopping service discovery for dashboard devices");
      nsdManager.stopServiceDiscovery(dashboardServiceDiscoveryListener);
    }

    // Do the inherited stuff
    super.stop();

  }
}
