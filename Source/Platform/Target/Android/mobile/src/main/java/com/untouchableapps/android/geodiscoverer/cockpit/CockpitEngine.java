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

package com.untouchableapps.android.geodiscoverer.cockpit;

import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.net.wifi.WifiManager;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitAppVibration;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class CockpitEngine extends com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitEngine {

  // Time info for deciding if silence needs to be played to wake up bluetooth device
  long audioWakeupLastTrigger = 0;
  long audioWakeupDelay = 0;
  long audioWakeupDuration = 0;

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
            audioWakeup();
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
      }

    });
    networkServerThread.start();
  }
  void stopNetworkService() {
    quitNetworkServerThread=true;
    Thread stopThread = new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          Socket clientSocket = new Socket("localhost",networkServerSocket.getLocalPort());
          clientSocket.getOutputStream().write(255);
          clientSocket.close();
          networkServerThread.join();
        }
        catch (Exception e) {
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

  // References for jmDNS
  WifiManager wifiManager;

  // References for the network discovery via ARP lookup
  Thread lookupARPCacheThread = null;
  boolean quitLookupARPCacheThread = false;

  /**
   * Returns the IP4 address of the wlan interface
   */
  @SuppressWarnings("unchecked")
  InetAddress getWifiInetAddress() {
    int ipAddress = wifiManager.getConnectionInfo().getIpAddress();
    if (ByteOrder.nativeOrder().equals(ByteOrder.LITTLE_ENDIAN)) {
      ipAddress = Integer.reverseBytes(ipAddress);
    }
    byte[] ipByteArray = BigInteger.valueOf(ipAddress).toByteArray();
    InetAddress inetAddress=null;
    try {
      inetAddress = InetAddress.getByAddress(ipByteArray);
    } catch (UnknownHostException e) {
      app.addMessage(app.DEBUG_MSG, "GDApp", e.getMessage());
    }
    return inetAddress;
  }

  /** Constructor */
  public CockpitEngine(GDApplication app) {

    // Call constructor
    super(app.getContext(),GDApplication.coreObject);

    // Remember context
    this.app = app;

    // Get services
    wifiManager = (WifiManager) app.getApplicationContext().getSystemService(android.content.Context.WIFI_SERVICE);

    // Init parameters
    audioWakeupDuration = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "audioWakeupDuration"))*1000;
    audioWakeupDelay = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "audioWakeupDelay"))*1000;

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

      // Use ARP cache lookups to discover devices if configured
      if (Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "useAddressCacheLookup"))!=0) {
        quitLookupARPCacheThread = false;
        app.addMessage(app.DEBUG_MSG, "GDApp", "starting ARP cache lookup thread");
        lookupARPCacheThread = new Thread(new Runnable() {

          @Override
          public void run() {

            int sleepTime = Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "addressCacheSleepTime"))*1000;
            int port = Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "port"));
            while (!quitLookupARPCacheThread) {

              // Go through the ARP cache
              try {
                BufferedReader br = new BufferedReader(new FileReader("/proc/net/arp"));
                try {
                  String line = br.readLine();
                  Pattern p = Pattern.compile("^\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s*");
                  while ((line != null)&&(!quitLookupARPCacheThread)) {
                    Matcher m = p.matcher(line);
                    if (m.find()) {
                      String ip = m.group(1);
                      app.coreObject.executeCoreCommand(String.format("addDashboardDevice(%s,%d)",ip,port));
                    }
                    line = br.readLine();
                  }
                } finally {
                  br.close();
                }
              }
              catch (IOException e) {
                app.addMessage(app.ERROR_MSG, "GDApp", e.toString());
                app.executeAppCommand("errorDialog(\"Could not lookup address cache!\")");
              }

              // Sleep for the defined time
              if (!quitLookupARPCacheThread) {
                try {
                  Thread.sleep(sleepTime);
                }
                catch (InterruptedException e) {
                }
              }
            }
          }
        });
        lookupARPCacheThread.start();
      }
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
    if (lookupARPCacheThread!=null) {
      app.addMessage(app.DEBUG_MSG, "GDApp", "stopping ARP cache lookup thread");
      quitLookupARPCacheThread = true;
      boolean repeat = true;
      while (repeat) {
        repeat=false;
        lookupARPCacheThread.interrupt();
        try {
          lookupARPCacheThread.join(100);
        }
        catch (InterruptedException e) {
          repeat=true;
        }
        if (lookupARPCacheThread.isAlive())
          repeat=true;
        else
          repeat=false;
      }
      lookupARPCacheThread = null;
    }

    // Do the inherited stuff
    super.stop();

  }

  /** Wakeups the audio device if required */
  public synchronized void audioWakeup() {
    long t = System.currentTimeMillis();
    if (audioIsAsleep()) {
      audioWakeupLastTrigger = t + audioWakeupDelay;
      try {
        final AssetFileDescriptor afd = app.getContext().getAssets().openFd("Sound/silence.mp3");
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
      boolean repeat=true;
      while (repeat) {
        try {
          Thread.sleep(audioWakeupLastTrigger-t);
          repeat=false;
        } catch (InterruptedException e) {
          t = System.currentTimeMillis();
          if (t>=audioWakeupLastTrigger)
            repeat=false;
        }
      }
    }
    audioWakeupLastTrigger = System.currentTimeMillis();
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("lastTrigger=%d",audioWakeupLastTrigger));
  }

  /** Indicates if audio device is asleep */
  public boolean audioIsAsleep() {
    long t = System.currentTimeMillis();
    return (t>audioWakeupLastTrigger+audioWakeupDuration);
  }
}
