package com.untouchableapps.android.geodiscoverer.logic.ble;
//============================================================================
// Name        : GDHeartRateService.java
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


import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.ParcelUuid;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class GDHeartRateService {

  // Import references
  private Context context;
  private BluetoothAdapter bluetoothAdapter;
  private BluetoothManager bluetoothManager;
  private GDCore coreObject;
  private BluetoothGatt bluetoothGatt=null;
  private BluetoothLeScanner bluetoothScanner=null;

  // Constants for finding heart rate services and measurements
  private final static UUID UUID_HEART_RATE_SERVICE = UUID.fromString("0000180d-0000-1000-8000-00805f9b34fb");
  private final static UUID UUID_HEART_RATE_MEASUREMENT_VALUE = UUID.fromString("00002a37-0000-1000-8000-00805f9b34fb");
  private final static UUID UUID_HEART_RATE_MEASUREMENT_DESCRIPTOR = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

  /** Address of the bluetooth device to monitor */
  String deviceAddress;

  // Heart rate info
  int currentHeartRate = 0;
  int currentHeartRateZone = 1;
  int maxHeartRate = Integer.MAX_VALUE;
  int startHeartRateZoneTwo = Integer.MAX_VALUE;
  int startHeartRateZoneThree = Integer.MAX_VALUE;
  int startHeartRateZoneFour = Integer.MAX_VALUE;
  int minHeartRateZoneChangeTime = 0;
  long heartRateZoneChangeTimestamp = 0;
  int volumeHeartRateZoneChange = 100;
  long connectionWarningPeriod = 0;
  long connectionWarningTimestamp = 0;
  public boolean heartRateUnavailableSound = true;

  // Thread playing audio if heart rate limit is reached
  Thread alarmThread = null;
  boolean quitAlarmThread = false;

  // Operation state
  private final int SCANNING = 0;
  private final int CONNECTING = 1;
  private final int DISCOVERING_SERVICES = 2;
  private final int OPERATING = 3;
  private int state = SCANNING;

  // List of known bluetooth devices
  public static LinkedList<String> knownDeviceAddresses = new LinkedList<String>();

  // Thread for handling deferred connection change indications
  private final long CONNECTION_RECOVERY_TIME = 10*1000;
  private Thread connectionNotificationThread = null;
  private final Lock connectionNotificationLock = new ReentrantLock();

  /** Extracts the heart rate from a characteristics */
  private void updateHeartRate(BluetoothGattCharacteristic characteristic) {
    int flag = characteristic.getProperties();
    int format = -1;
    if ((flag & 0x01) != 0) {
      format = BluetoothGattCharacteristic.FORMAT_UINT16;
    } else {
      format = BluetoothGattCharacteristic.FORMAT_UINT8;
    }
    currentHeartRate = characteristic.getIntValue(format, 1);
    //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR", String.format("received heart rate: %d", currentHeartRate));
  }

  /** Callback for gatt service updates */
  private BluetoothGattCallback gattCallback = new BluetoothGattCallback() {

    /** Called when connection to the device has changed */
    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
      super.onConnectionStateChange(gatt,status,newState);
      String intentAction;
      if (newState == BluetoothProfile.STATE_CONNECTED) {
        if (state==CONNECTING) {
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDAppHR", "connection to bluetooth gatt service established, requesting service discovery");
          gatt.discoverServices();
          state = DISCOVERING_SERVICES;
        }
      } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR","connection to bluetooth gatt service dropped");
        state = CONNECTING;
        setConnectionState(false);
      }
    }

    /** Called when new services are discovered */
    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
      super.onServicesDiscovered(gatt,status);
      if (status == BluetoothGatt.GATT_SUCCESS) {
        if (state == DISCOVERING_SERVICES) {
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR","bluetooth gatt service discovery completed");
          List<BluetoothGattService> gattServices = gatt.getServices();
          if (gattServices == null) return;
          for (BluetoothGattService gattService : gattServices) {
            //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR",String.format("device supports service %s",gattService.getUuid().toString()));
            if (gattService.getUuid().equals(UUID_HEART_RATE_SERVICE)) {
              List<BluetoothGattCharacteristic> gattCharacteristics = gattService.getCharacteristics();
              if (gattCharacteristics == null) return;
              for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR",String.format("heart rate service supports characteristics %s",gattCharacteristic.getUuid().toString()));
                if (gattCharacteristic.getUuid().equals(UUID_HEART_RATE_MEASUREMENT_VALUE)) {
                  GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR",String.format("requesting read of characteristic",gattCharacteristic.getUuid().toString()));
                  gatt.readCharacteristic(gattCharacteristic);
                  gatt.setCharacteristicNotification(gattCharacteristic,true);
                  BluetoothGattDescriptor descriptor = gattCharacteristic.getDescriptor(UUID_HEART_RATE_MEASUREMENT_DESCRIPTOR);
                  descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                  gatt.writeDescriptor(descriptor);
                  state = OPERATING;
                  setConnectionState(true);
                }
              }
            }
          }
        }
      }
    }

    // Result of a characteristic read operation
    @Override
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic,
                                     int status) {
      super.onCharacteristicRead(gatt, characteristic, status);
      if (status == BluetoothGatt.GATT_SUCCESS) {
        if (characteristic.getUuid().equals(UUID_HEART_RATE_MEASUREMENT_VALUE)) {
          updateHeartRate(characteristic);
        }
      }
    }

    // Result of a characteristic update operation
    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
      super.onCharacteristicChanged(gatt, characteristic);
      if (characteristic.getUuid().equals(UUID_HEART_RATE_MEASUREMENT_VALUE)) {
        updateHeartRate(characteristic);
      }
    }
  };

  /** Callback for bluetooth devices discoveries  */
  private ScanCallback scanCallback = new ScanCallback() {

    @Override
    public void onScanResult(int callbackType, ScanResult result) {
      super.onScanResult(callbackType, result);
      if (result==null)
        return;
      if (knownDeviceAddresses!=null) {
        boolean found=false;
        for (String knownDeviceAddress : knownDeviceAddresses) {
          if (knownDeviceAddress.equals(result.getDevice().getAddress())) {
            found=true;
          }
        }
        if (!found) {
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppHR", String.format("new bluetooth le device <%s> discovered!",result.getDevice().getAddress()));
          knownDeviceAddresses.add(result.getDevice().getAddress());
        }
      }
      if (result.getDevice().getAddress().equals(deviceAddress)) {
        if (state==SCANNING) {
          bluetoothGatt = result.getDevice().connectGatt(context, true, gattCallback);
          state=CONNECTING;
          bluetoothScanner.stopScan(this);
        }
      }
    }
  };

  /** Checks if bluetooth le is supported on this device */
  static public boolean isSupported(Context context, GDCore coreObject) {
    if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
      if (Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "active"))!=0)
        return true;
      else
        return false;
    } else {
      return false;
    }
  }

  /** Creates a filter list for scanning */
  private List<ScanFilter> createScanFilterList(UUID[] uuids) {
    List<ScanFilter> filterList = new ArrayList<>();
    for (UUID uuid : uuids) {
      ScanFilter filter = new ScanFilter.Builder()
          .setServiceUuid(new ParcelUuid(uuid))
          .build();
      filterList.add(filter);
    };
    return filterList;
  }

  /** Constructor */
  public GDHeartRateService(Context context, GDCore coreObject) {

    // Store important references
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","starting service");
    this.context = context;
    this.coreObject = coreObject;
    deviceAddress = coreObject.configStoreGetStringValue("HeartRateMonitor", "bluetoothAddress");
    if (!knownDeviceAddresses.contains(deviceAddress))
      knownDeviceAddresses.add(deviceAddress);
    maxHeartRate = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "maxHeartRate"));
    startHeartRateZoneTwo = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "startHeartRateZoneTwo"));
    startHeartRateZoneThree = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "startHeartRateZoneThree"));
    startHeartRateZoneFour = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "startHeartRateZoneFour"));
    minHeartRateZoneChangeTime = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "minHeartRateZoneChangeTime"));
    volumeHeartRateZoneChange = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "volumeHeartRateZoneChange"));
    connectionWarningPeriod = Integer.parseInt(coreObject.configStoreGetStringValue("HeartRateMonitor", "connectionWarningPeriod"));
    connectionWarningTimestamp = System.currentTimeMillis() / 1000;

    // Setup bluetooth low energy
    bluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
    bluetoothAdapter = bluetoothManager.getAdapter();
    final UUID[] uuids = {UUID_HEART_RATE_SERVICE};
    List<ScanFilter> filterList = createScanFilterList(uuids);
    ScanSettings settings = new ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_BALANCED)
        .build();
    bluetoothScanner = bluetoothAdapter.getBluetoothLeScanner();
    bluetoothScanner.startScan(filterList,settings,scanCallback);

    // Start alarm thread
    alarmThread = new Thread(new Runnable() {
      MediaPlayer player = null;
      MediaPlayer player2 = null;
      AssetFileDescriptor afd = null;
      final Lock playerLock = new ReentrantLock();
      boolean alarmPlaying = false;
      @Override
      public void run() {
        coreObject.setThreadPriority(2);
        while (!quitAlarmThread) {

          // If heart rate is not available, issue an alarm from time to time
          if (state!=OPERATING) {

            if ((heartRateUnavailableSound)&&(connectionNotificationThread==null)) {
              long t = System.currentTimeMillis() / 1000;
              if (t >= connectionWarningTimestamp + connectionWarningPeriod) {
                coreObject.playSound("heartRateUnavailable.ogg", 1, 100);
                connectionWarningTimestamp = t;
              }
            }

          } else {

            // Handle the heart rate alarm
            try {

              // Alarm already playing?
              if (player != null) {

                // Stop if heart rate is below maximum
                if (currentHeartRate <= maxHeartRate) {
                  playerLock.lock();
                  player.stop();
                  player2.stop();
                  alarmPlaying = false;
                  playerLock.unlock();
                }

                // Clean up if player is stopped
                if ((!player.isPlaying()) && (!player2.isPlaying())) {
                  player.release();
                  player2.release();
                  afd.close();
                  player = null;
                  player2 = null;
                }

              } else {

                // Start playing if heart rate is above maximum
                if (currentHeartRate > maxHeartRate) {
                  afd = context.getAssets().openFd("Sound/heartRateAlarm.ogg");
                  player = new MediaPlayer();
                  player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                  player.prepare();
                  player2 = new MediaPlayer();
                  player2.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                  player2.prepare();
                  player.setNextMediaPlayer(player2);
                  player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    @Override
                    public void onCompletion(MediaPlayer mp) {
                      playerLock.lock();
                      if (alarmPlaying) {
                        player.reset();
                        try {
                          player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                          player.prepare();
                        } catch (IOException e) {
                          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDAppHR", e.getMessage());
                        }
                        player2.setNextMediaPlayer(player);
                        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","player done, handing over to player2");
                      }
                      playerLock.unlock();
                    }
                  });
                  player2.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    @Override
                    public void onCompletion(MediaPlayer mp) {
                      playerLock.lock();
                      if (alarmPlaying) {
                        player2.reset();
                        try {
                          player2.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                          player2.prepare();
                        } catch (IOException e) {
                          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDAppHR", e.getMessage());
                        }
                        player.setNextMediaPlayer(player2);
                        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","player2 done, handing over to player");
                      }
                      playerLock.unlock();
                    }
                  });
                  alarmPlaying = true;
                  player.start();

                }
              }
            } catch (IOException e) {
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDAppHR", e.getMessage());
            }

            // Find out the zone the heart rate is in
            long t = System.currentTimeMillis() / 1000;
            int nextHeartRateZone = 4;
            if (currentHeartRate < startHeartRateZoneTwo)
              nextHeartRateZone = 1;
            else if (currentHeartRate < startHeartRateZoneThree)
              nextHeartRateZone = 2;
            else if (currentHeartRate < startHeartRateZoneFour)
              nextHeartRateZone = 3;
            if (currentHeartRateZone != nextHeartRateZone) {
              if (heartRateZoneChangeTimestamp == 0) {
                heartRateZoneChangeTimestamp = t;
              }
              if (t - heartRateZoneChangeTimestamp >= minHeartRateZoneChangeTime) {
                coreObject.playSound("heartRateZoneChange.ogg", nextHeartRateZone, volumeHeartRateZoneChange);
                currentHeartRateZone = nextHeartRateZone;
                heartRateZoneChangeTimestamp = 0;
              }
            } else {
              heartRateZoneChangeTimestamp = 0;
            }
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR",String.format("current heart rate zone: %d",currentHeartRateZone));
          }

          // Wait for next round
          try {
            Thread.sleep(1000);
          }
          catch (InterruptedException e) {
          }
        }

        // Stop the player if it's still running
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","shutting down media player");
        playerLock.lock();
        alarmPlaying=false;
        if (player!=null) {
          player.stop();
          player.release();
          player=null;
        }
        if (player2!=null) {
          player2.stop();
          player2.release();
          player2=null;
        }
        if (afd!=null) {
          try {
            afd.close();
          }
          catch(IOException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDAppHR", e.getMessage());
          }
          afd=null;
        }
        playerLock.unlock();
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","exiting alarm thread");
      }
    });
    alarmThread.start();
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","alarm thread started");
  }

  private void signalConnectionChange(String soundFile) {
    coreObject.playSound(soundFile, 1, 100);
  }

  private void setConnectionState(boolean connected) {

    // Stop last notification thread (if running)
    connectionNotificationLock.lock();
    if (connectionNotificationThread != null) {
      connectionNotificationThread.interrupt();
      connectionNotificationThread=null;
      if (connected) {
        connectionNotificationLock.unlock();
        return;
      }
    }
    connectionNotificationLock.unlock();

    // Define what state change it is
    final String soundFile;
    if (connected) {
      soundFile="heartRateConnect.ogg";
    } else {
      soundFile="heartRateDisconnect.ogg";
    }

    // Defer the state change in case it changes soon again
    if (!connected) {
      connectionNotificationThread = new Thread() {
        public void run() {
          coreObject.setThreadPriority(2);
          try {
            Thread.sleep(CONNECTION_RECOVERY_TIME);
          } catch (InterruptedException e) {
            return;
          }
          connectionNotificationLock.lock();
          signalConnectionChange(soundFile);
          connectionNotificationThread=null;
          connectionNotificationLock.unlock();
        }
      };
      connectionNotificationThread.start();
    } else {
      connectionNotificationLock.lock();
      signalConnectionChange(soundFile);
      connectionNotificationLock.unlock();
    }
  }

  /** Stops all services */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
  public void deinit() {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppHR","stopping service");
    if (bluetoothScanner !=null) {
      bluetoothScanner.stopScan(scanCallback);
    }
    if (bluetoothGatt!=null) {
      bluetoothGatt.close();
      bluetoothGatt=null;
    }
    quitAlarmThread = true;
  }

}
