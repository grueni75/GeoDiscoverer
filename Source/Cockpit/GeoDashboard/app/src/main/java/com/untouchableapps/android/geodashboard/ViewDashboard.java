//============================================================================
// Name        : ViewDashboard.java
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

package com.untouchableapps.android.geodashboard;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigInteger;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.Enumeration;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceInfo;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
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
  private int updateCount=0;
  private final int FULL_REFRESH_UPDATE_COUNT=60;

  /**
   * Indicates that the network service shall be restarted
   */
  boolean restartNetworkService = false;

  /**
   * Indicates that WLAN is active
   */
  boolean wlanActive = false;

  /**
   * Indicates that the app is active
   */
  boolean appActive = false;

  /**
   * Indicates that the service is active
   */
  boolean serviceActive = false;

  /**
   * Handle to the thread that monitors state changes
   */
  Thread monitorThread = null;

  /**
   * References to the full screen image view
   */
  ImageView dashboardView;

  /**
   * References to the sound buttons bar
   */
  LinearLayout soundButtonsView;

  /**
   * References to the backlight indicator image view
   */
  ImageView backlightIndicatorView;

  /**
   * Resolver for system settings
   */
  ContentResolver systemSettingsResolver;

  /**
   * Observer for system settings
   */
  ContentObserver systemSettingsObserver;

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

  /**
   * Port to listen to
   */
  int serverPort = 11111;

  /**
   * Internet address of last client
   */
  String lastClientAddress = null;

  // Actions for message handling
  static final int ACTION_DISPLAY_BITMAP = 0;
  static final int ACTION_DISPLAY_TOAST = 1;

  // Network commands
  static final int NET_CMD_GET_INFO = 1;
  static final int NET_CMD_DISPLAY_BITMAP = 2;
  static final int NET_CMD_PLAY_SOUND_DOG = 3;
  static final int NET_CMD_PLAY_SOUND_SHIP = 4;
  static final int NET_CMD_PLAY_SOUND_CAR = 5;

  /**
   * Registers the network service of this app
   */
  private boolean registerService(int port) {

    ServiceInfo serviceInfo = ServiceInfo.create(
        "_geodashboard._tcp.",
        "GeoDashboard", port,
        "Displays dashboard images from Geo Discoverer"
    );
    try {
      jmDNS.registerService(serviceInfo);
    }
    catch (IOException e) {
      Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
      msg.sendToTarget();
      return false;
    }
    return true;
  }

  /**
   * Returns the IP4 address of the wlan interface
   */
  @SuppressWarnings("unchecked")
  private InetAddress getWifiInetAddress() {
    int ipAddress = wifiManager.getConnectionInfo().getIpAddress();
    if (ByteOrder.nativeOrder().equals(ByteOrder.LITTLE_ENDIAN)) {
      ipAddress = Integer.reverseBytes(ipAddress);
    }
    byte[] ipByteArray = BigInteger.valueOf(ipAddress).toByteArray();
    InetAddress inetAddress=null;
    try {
      inetAddress = InetAddress.getByAddress(ipByteArray);
    } catch (UnknownHostException e) {
      //Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
    }
    return inetAddress;
  }

  /**
   * Creates a bitmap with the given text
   */
  private Bitmap createTextBitmap(String text) {
    Paint paint = new Paint();
    paint.setTextSize(22 * getResources().getDisplayMetrics().density);
    paint.setColor(Color.BLACK);
    paint.setTextAlign(Paint.Align.CENTER);
    Bitmap image = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(image);
    canvas.drawText(text, width/2, height/2, paint);
    return image;
  }

  /**
   * Displays a bitmap with a text
   */
  protected void displayTextBitmap(String text) {
    Bitmap b = createTextBitmap(text);
    Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,b);
    msg.sendToTarget();
  }

  /** Indicates if backlight is on or off
   *
   */
  protected void updateBacklightIndicator() {

    // Read the sys entry to get the current led brightness
    final int brightnessLevel;
    File file = new File("/sys/devices/platform/mxc_msp430_fl.0/backlight/mxc_msp430_fl.0/actual_brightness");
    int t=0;
    try {
      BufferedReader br = new BufferedReader(new FileReader(file));
      String line=br.readLine();
      br.close();
      t=Integer.valueOf(line);
    }
    catch (IOException e) {
      Log.d("GeoDashboard", String.format(e.getMessage()));
    }
    brightnessLevel=t;
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        //Toast.makeText(ViewDashboard.this,String.valueOf(brightnessLevel),Toast.LENGTH_LONG).show();
        if (brightnessLevel != 0)
          backlightIndicatorView.setVisibility(View.VISIBLE);
        else
          backlightIndicatorView.setVisibility(View.INVISIBLE);
      }
    });
  }


  /**
   * Sends a message to the client
   */
  class SendMessageOnClickListener implements View.OnClickListener {
    int cmd;
    SendMessageOnClickListener(int cmd) {
      this.cmd=cmd;
    }
    @Override
    public void onClick(View v) {
      Thread networkThread = new Thread(new Runnable() {

        @Override
        public void run() {

          try {
            if (lastClientAddress != null) {
              Socket s = new Socket(lastClientAddress, serverPort+1);
              s.getOutputStream().write(cmd);
              s.close();
            }
          } catch (IOException e) {
            Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
            msg.sendToTarget();
          }

        }

      });
      networkThread.start();
    }
  }

  /**
   * Starts the network service
   */
  private void startService(boolean wait) {

    quitServerThread=false;
    serverThread = new Thread(new Runnable() {

      @Override
      public void run() {

        // Open server socket
        if (serverSocket==null) {
          try {
            serverSocket = new ServerSocket();
            serverSocket.setReuseAddress(true);
            serverSocket.bind(new InetSocketAddress(serverPort));
          }
          catch (IOException e) {
            Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
            msg.sendToTarget();
            displayTextBitmap(getString(R.string.server_down));
            serverSocket = null;
            return;
          }
        }

        // Create the JmDNS object
        InetAddress address = getWifiInetAddress();
        if (address==null) {
          Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,getString(R.string.no_wlan_address));
          msg.sendToTarget();
          displayTextBitmap(getString(R.string.server_down));
          return;
        }
        try {
          jmDNS = JmDNS.create(address);
        }
        catch (IOException e) {
          Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
          msg.sendToTarget();
          displayTextBitmap(getString(R.string.server_down));
          return;
        }

        // Register the service
        if (!registerService(serverSocket.getLocalPort())) {
          displayTextBitmap(getString(R.string.server_down));
          return;
        }

        // Set bitmap to indicate "wait for first connection"
        Bitmap b = createTextBitmap(getString(R.string.waiting_for_connection,address.toString(),serverSocket.getLocalPort()));
        Message msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,b);
        msg.sendToTarget();
        b=null;
        serviceActive = true;

        // Handle network communication
        while (!quitServerThread) {
          try {
            Socket client = serverSocket.accept();
            lastClientAddress = client.getInetAddress().getHostAddress();
            int cmd = client.getInputStream().read();
            switch(cmd) {
              case NET_CMD_GET_INFO:
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                DataOutputStream dos = new DataOutputStream(baos);
                dos.writeInt(orientation);
                dos.writeInt(width);
                dos.writeInt(height);
                client.getOutputStream().write(baos.toByteArray());
                baos = null;
                dos = null;
                break;
              case NET_CMD_DISPLAY_BITMAP:
                Bitmap dashboardBitmap = BitmapFactory.decodeStream(client.getInputStream());
                msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_BITMAP,dashboardBitmap);
                msg.sendToTarget();
                dashboardBitmap = null;
                break;
            }
            client.close();
          }
          catch (IOException e) {
            msg = serverThreadHandler.obtainMessage(ACTION_DISPLAY_TOAST,e.getMessage());
            msg.sendToTarget();
          }
        }

        // Unregister the service
        jmDNS.unregisterAllServices();
        try {
          jmDNS.close();
        }
        catch (IOException e) {
          Toast.makeText(ViewDashboard.this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
        displayTextBitmap(getString(R.string.server_down));

      }

    });
    serverThread.start();

    // Display text to indicate initialization
    displayTextBitmap(getString(R.string.init_server));

    // Shall we wait
    if (wait) {
      while (!serviceActive) {
        try {
          Thread.sleep(1000);
        }
        catch (InterruptedException e) {
        }
      }
    }
  }

  /**
   * Stops the network service
   */
  private void stopService(boolean wait) {
    quitServerThread=true;
    displayTextBitmap(getString(R.string.deinit_server));
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
        displayTextBitmap(getString(R.string.server_down));
        serviceActive=false;
        if (restartNetworkService) {
          restartNetworkService=false;
          startService(false);
        }
      }
    });
    stopThread.start();
    if (wait) {
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
  }

  /**
   * Restarts the network service
   */
  protected void restartService() {
    //restartNetworkService=true;
    stopService(false);
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
   * Tells all other threads to quit
   */
  private boolean quitThreads = false;

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

    // Set the content
    setContentView(R.layout.view_dashboard);
    dashboardView = (ImageView)findViewById(R.id.fullscreen_content);
    soundButtonsView = (LinearLayout)findViewById(R.id.sound_buttons);
    backlightIndicatorView = (ImageView)findViewById(R.id.backlight_indicator);

    // Keep the screen on
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

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
            soundButtonsView.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
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

    // Handle messages from the server thread
    serverThreadHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message inputMessage) {
        switch(inputMessage.what) {
          case ViewDashboard.ACTION_DISPLAY_BITMAP:
            updateCount++;
            long delay=0;
            if (updateCount>=FULL_REFRESH_UPDATE_COUNT) {
              Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
              Canvas canvas = new Canvas(bitmap);
              Paint paint = new Paint();
              paint.setColor(Color.BLACK);
              canvas.drawRect(0F, 0F, (float) width, (float) height, paint);;
              dashboardView.setImageBitmap(bitmap);
              updateCount=0;
              delay=100;
            }
            final Handler handler = new Handler(Looper.getMainLooper());
            final Bitmap dashboardBitmap = (Bitmap) inputMessage.obj;
            handler.postDelayed(new Runnable() {
              @Override
              public void run() {
                dashboardView.setImageBitmap(dashboardBitmap);
              }
            }, delay);
            break;
          case ViewDashboard.ACTION_DISPLAY_TOAST:
            String message = (String) inputMessage.obj;
            Toast.makeText(ViewDashboard.this, message, Toast.LENGTH_LONG).show();
            break;
        }
      }
    };

    // Get notifications if WLAN changes
    IntentFilter intentFilter = new IntentFilter();
    intentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
    registerReceiver(new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        //Log.d("GeoDashboard","WLAN state has changed!");
        boolean connectionEstablished = intent.getBooleanExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED, false);
        if (connectionEstablished) {
          wlanActive = true;
        } else {
          wlanActive = false;
        }
      }

    }, intentFilter);

    // Start thread that handles wlan and backlight changes
    wifiManager = (android.net.wifi.WifiManager) getSystemService(android.content.Context.WIFI_SERVICE);
    monitorThread = new Thread(new Runnable() {

      @Override
      public void run() {

        // Endless loop
        while (!quitThreads) {

          // Handle waln changes
          //Log.d("GeoDashboard",String.format("appActive=%s wlanActive=%s",Boolean.toString(appActive),Boolean.toString(wlanActive)));
          if (appActive) {
            if (!serviceActive) {
              InetAddress address = getWifiInetAddress();
              address = getWifiInetAddress();
              if (address!=null) {
                wlanActive=true;
                startService(true);
              }
            }
          }
          if ((!wlanActive)||(!appActive)) {
            if (serviceActive) {
              stopService(true);
            }
          }

          // Handle brightness changes
          if (appActive) {
            updateBacklightIndicator();
          }

          // Wait for the next round
          try {
            Thread.sleep(1000);
          }
          catch (InterruptedException e) {
          }
        }
      }
    });
    monitorThread.start();

    // Service is down
    displayTextBitmap(getString(R.string.server_down));

    // Set the onclick handlers for the buttons
    ((ImageButton)findViewById(R.id.sound_button_car)).setOnClickListener(new
            SendMessageOnClickListener(NET_CMD_PLAY_SOUND_CAR)
    );
    ((ImageButton)findViewById(R.id.sound_button_dog)).setOnClickListener(new
            SendMessageOnClickListener(NET_CMD_PLAY_SOUND_DOG)
    );
    ((ImageButton)findViewById(R.id.sound_button_ship)).setOnClickListener(new
            SendMessageOnClickListener(NET_CMD_PLAY_SOUND_SHIP)
    );
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
   * Called when activity is resumed
   */
  @Override
  protected void onResume() {
    super.onResume();

    // Enable multicast on WLAN
    multicastLock = wifiManager.createMulticastLock("Geo Dashboard lock for JmDNS");
    multicastLock.setReferenceCounted(true);
    multicastLock.acquire();

    // App is available
    appActive=true;
  }

  /**
   * Called when the activity is paused
   */
  @Override
  protected void onPause() {
    super.onPause();

    // App is not available
    appActive=false;

    // Release the multicast lock
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
        restartService();
        return true;
      case R.id.action_about:
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder
            .setTitle(R.string.about_title)
            .setMessage(R.string.about_message)
            .setCancelable(true)
            .setNeutralButton(R.string.about_dismiss,new DialogInterface.OnClickListener() {
              public void onClick(DialogInterface dialog,int id) {
                dialog.cancel();
              }
            });
        AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.show();
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }

  /** Close the socket on destroy */
  @Override
  protected void onDestroy() {
    quitThreads=true;
    if (monitorThread!=null) {
      try {
        monitorThread.join();
      }
      catch (Exception e) {
      }
    }
    stopService(false);
    while (serviceActive) {
      try {
        Thread.sleep(1000);
      }
      catch (InterruptedException e) {
      }
    }
    if (serverSocket!=null) {
      try {
        serverSocket.close();
      }
      catch (IOException e) {
      }
      serverSocket=null;
    }
    Toast.makeText(this,R.string.quit_message, Toast.LENGTH_LONG).show();
    super.onDestroy();
  }

}
