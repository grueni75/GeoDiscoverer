//============================================================================
// Name        : ViewDashboard.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodashboard;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Arrays;
import java.util.Enumeration;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceInfo;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.format.Formatter;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.Toast;

import com.untouchableapps.android.geodashboard.util.SystemUiHider;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 *
 * @see SystemUiHider
 */
public class ViewDashboard extends Activity {

  /**
   * Whether or not the system UI should be auto-hidden after
   * {@link #AUTO_HIDE_DELAY_MILLIS} milliseconds.
   */
  private static final boolean AUTO_HIDE = true;

  /**
   * If {@link #AUTO_HIDE} is set, the number of milliseconds to wait after user
   * interaction before hiding the system UI.
   */
  private static final int AUTO_HIDE_DELAY_MILLIS = 3000;

  /**
   * If set, will toggle the system UI visibility upon interaction. Otherwise,
   * will show the system UI visibility upon interaction.
   */
  private static final boolean TOGGLE_ON_CLICK = true;

  /**
   * The flags to pass to {@link SystemUiHider#getInstance}.
   */
  private static final int HIDER_FLAGS = SystemUiHider.FLAG_HIDE_NAVIGATION;
    
  // Infos about the screen
  private int orientation = 1;  // from Screen.h in Geo Discoverer
  private int width;
  private int height;
  
  /**
   * Indicates that the network service shall be restarted
   */
  boolean restartNetworkService = false;
  
  /**
   * References to the full screen image view
   */
  ImageView dashboardView;
  
  /**
   * Handler that receives message from the server thread
   */
  Handler serverThreadHandler;
  
  /**
   * Lock for multicast on WLAN
   */
  MulticastLock multicastLock;
  
  /**
   * The JmDNS object
   */
  JmDNS jmDNS;
  
  /**
   * WiFi system service
   */
  WifiManager wifiManager;
  
  // Actions for message handling
  static final int ACTION_DISPLAY_BITMAP = 0;
  static final int ACTION_DISPLAY_TOAST = 1;
  
  /**
   * Registers the network service of this app
   */
  private void registerService(int port) {
        
    ServiceInfo serviceInfo = ServiceInfo.create(
        "_geodashboard._tcp.",
        "GeoDashboard", port,
        "Displays dashboard images from Geo Discoverer"
    );
    try {
      jmDNS.registerService(serviceInfo);
    }
    catch (IOException e) {
      Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
    }
  }

  /**
   * Finds all IP addresses of the wlan adapter
   */
  private Enumeration<InetAddress> getWifiInetAddresses(final Context context) {
    final WifiInfo wifiInfo = wifiManager.getConnectionInfo();
    final String macAddress = wifiInfo.getMacAddress();
    final String[] macParts = macAddress.split(":");
    final byte[] macBytes = new byte[macParts.length];
    for (int i = 0; i< macParts.length; i++) {
      macBytes[i] = (byte)Integer.parseInt(macParts[i], 16);
    }
    try {
      final Enumeration<NetworkInterface> e =  NetworkInterface.getNetworkInterfaces();
      while (e.hasMoreElements()) {
        final NetworkInterface networkInterface = e.nextElement();
        if (Arrays.equals(networkInterface.getHardwareAddress(), macBytes)) {
          return networkInterface.getInetAddresses();
        }
      }
    } catch (SocketException e) {
      Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
    }
    return null;
  }

  /**
   * Returns the requested IP address class 
   */
  @SuppressWarnings("unchecked")
  private <T extends InetAddress> T getWifiInetAddress(final Context context, final Class<T> inetClass) {
      final Enumeration<InetAddress> e = getWifiInetAddresses(context);
      while (e.hasMoreElements()) {
          final InetAddress inetAddress = e.nextElement();
          if (inetAddress.getClass() == inetClass) {
              return (T)inetAddress;
          }
      }
      return null;
  }

  /**
   * Creates a bitmap with the given text
   */
  private Bitmap createTextBitmap(String text) {
    Paint paint = new Paint();
    paint.setTextSize(16 * getResources().getDisplayMetrics().density);
    paint.setColor(Color.BLACK);
    paint.setTextAlign(Paint.Align.CENTER);
    Bitmap image = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(image);
    canvas.drawText(text, width/2, height/2, paint);
    return image;
  }
  
  /**
   * Starts the network service
   */
  private void startService() {
        
    quitServerThread=false;
    serverThread = new Thread(new Runnable() {
      
      @Override
      public void run() {

        // Open server socket
        try {
          serverSocket = new ServerSocket(11111);
        }
        catch (IOException e) {
          Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
          msg.sendToTarget();
          return;
        }
        if (serverSocket==null) 
          return;

        // Create the JmDNS object
        InetAddress address = getWifiInetAddress(ViewDashboard.this, Inet4Address.class);
        try {
          jmDNS = JmDNS.create(address);
        }
        catch (IOException e) {
          Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
          msg.sendToTarget();
          return;
        }

        // Register the service
        registerService(serverSocket.getLocalPort());

        // Set bitmap to indicate "wait for first connection"
        Bitmap b = createTextBitmap(getString(R.string.waiting_for_connection,address.toString(),serverSocket.getLocalPort()));
        Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,b);
        msg.sendToTarget();
        b=null;

        // Handle network communication
        while (!quitServerThread) {
          try {
            Socket client = serverSocket.accept();
            int cmd = client.getInputStream().read();
            switch(cmd) {
              case 1: 
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                DataOutputStream dos = new DataOutputStream(baos);
                dos.writeInt(orientation);  
                dos.writeInt(width);  
                dos.writeInt(height);  
                client.getOutputStream().write(baos.toByteArray());
                baos = null;
                dos = null;
                break;
              case 2:
                Bitmap dashboardBitmap = BitmapFactory.decodeStream(client.getInputStream());
                msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,dashboardBitmap);
                msg.sendToTarget();
                dashboardBitmap = null;
                break;
            }
            client.close();
          }
          catch (IOException e) {
            Toast.makeText(ViewDashboard.this, e.getMessage(), Toast.LENGTH_LONG).show();
          }
        }
        
        // Unregister the service
        jmDNS.unregisterAllServices();
        try {
          jmDNS.close();
          serverSocket.close();
        }
        catch (IOException e) {
          Toast.makeText(ViewDashboard.this, e.getMessage(), Toast.LENGTH_LONG).show();
          return;
        }
        
      }
      
    });
    serverThread.start();
    
    // Display text to indicate initialization
    Bitmap b = createTextBitmap(getString(R.string.init_server));
    Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,b);
    msg.sendToTarget();
    b=null;

  }
  
  /**
   * Stops the network service
   */
  private void stopService() {
    quitServerThread=true;
    Bitmap b = createTextBitmap(getString(R.string.deinit_server));
    Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,b);
    msg.sendToTarget();
    b=null;    
    Thread stopThread = new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          Socket clientSocket = new Socket("localhost",serverSocket.getLocalPort());
          clientSocket.getOutputStream().write(255);
          clientSocket.close();
          serverThread.join();
        }
        catch (Exception e) {
        }
        if (restartNetworkService) {
          restartNetworkService=false;
          startService();
        }
      }
    });
    stopThread.start();
  }
  
  /**
   * Socket that receives the communication with geo discoverer
   */
  ServerSocket serverSocket = null;
  
  /** 
   * Tells the server thread that it shall quit
   */
  private boolean quitServerThread = false;
  
  /**
   * Thread that handles the communication with geo discoverer
   */
  private Thread serverThread = null;
  
  /**
   * The instance of the {@link SystemUiHider} for this activity.
   */
  private SystemUiHider mSystemUiHider;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setContentView(R.layout.view_dashboard);

    dashboardView = (ImageView)findViewById(R.id.fullscreen_content);
    
    // Set up an instance of SystemUiHider to control the system UI for
    // this activity.
    mSystemUiHider = SystemUiHider.getInstance(this, dashboardView, HIDER_FLAGS);
    mSystemUiHider.setup();
    mSystemUiHider
        .setOnVisibilityChangeListener(new SystemUiHider.OnVisibilityChangeListener() {

          @Override
          @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
          public void onVisibilityChange(boolean visible) {

            if (visible && AUTO_HIDE) {
              // Schedule a hide().
              delayedHide(AUTO_HIDE_DELAY_MILLIS);
            }
          }
        });

    // Set up the user interaction to manually show or hide the system UI.
    dashboardView.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View view) {
        if (TOGGLE_ON_CLICK) {
          mSystemUiHider.toggle();
        } else {
          mSystemUiHider.show();
        }
      }
    });
        
    // Update the properties of the device
    WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
    Display display = wm.getDefaultDisplay();
    Point size = new Point(0,0);
    display.getRealSize(size);
    width = size.x;
    height = size.y;

    // Enable multicast on WLAN
    wifiManager = (android.net.wifi.WifiManager) getSystemService(android.content.Context.WIFI_SERVICE);
    multicastLock = wifiManager.createMulticastLock("Geo Dashboard lock for JmDNS");
    multicastLock.setReferenceCounted(true);
    multicastLock.acquire();
        
    // Handle messages from the server thread
    serverThreadHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message inputMessage) {
        switch(inputMessage.what) {
          case ViewDashboard.ACTION_DISPLAY_BITMAP:
            Bitmap dashboardBitmap = (Bitmap) inputMessage.obj;
            dashboardView.setImageBitmap(dashboardBitmap);
            dashboardBitmap = null;
            break;
          case ViewDashboard.ACTION_DISPLAY_TOAST:
            String message = (String) inputMessage.obj;
            Toast.makeText(ViewDashboard.this, message, Toast.LENGTH_LONG).show();
            break;          
        }
      }
    };
    
    // Create the network service
    startService();
  }

  @Override
  protected void onPostCreate(Bundle savedInstanceState) {
    super.onPostCreate(savedInstanceState);

    // Trigger the initial hide() shortly after the activity has been
    // created, to briefly hint to the user that UI controls
    // are available.
    delayedHide(100);
  }

  Handler mHideHandler = new Handler();
  Runnable mHideRunnable = new Runnable() {
    @Override
    public void run() {
      mSystemUiHider.hide();
    }
  };
  
  /**
   * Schedules a call to hide() in [delay] milliseconds, canceling any
   * previously scheduled calls.
   */
  private void delayedHide(int delayMillis) {
    mHideHandler.removeCallbacks(mHideRunnable);
    mHideHandler.postDelayed(mHideRunnable, delayMillis);
  }
  
  /**
   * Suspend the network service
   */
  @Override
  protected void onDestroy() {
    super.onDestroy();
    stopService();
    if (multicastLock!=null)
      multicastLock.release();
  }
  
  /** Create the action buttons */
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu items for use in the action bar
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.view_dashboard_actions, menu);
    return super.onCreateOptionsMenu(menu);
  }
  
  /** Respond to action buttons */
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.action_refresh:
        restartNetworkService=true;
        stopService();
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }
  
}