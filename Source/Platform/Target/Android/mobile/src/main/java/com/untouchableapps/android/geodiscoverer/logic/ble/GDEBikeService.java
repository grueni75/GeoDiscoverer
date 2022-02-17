package com.untouchableapps.android.geodiscoverer.logic.ble;
//============================================================================
// Name        : GDEBikeService.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2020 Matthias Gruenewald
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
import android.bluetooth.BluetoothDevice;
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
import android.os.Build;
import android.os.ParcelUuid;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.UUID;

public class GDEBikeService {

  // Import references
  private Context context;
  private BluetoothAdapter bluetoothAdapter;
  private BluetoothManager bluetoothManager;
  private GDCore coreObject;
  private BluetoothGatt bluetoothGatt=null;
  private BluetoothLeScanner bluetoothScanner=null;

  // Constants for finding ebike services and measurements
  private final static UUID UUID_ADDE_SERVICE = UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb");
  private final static UUID UUID_ADDE_STATUS_VALUE = UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb");
  private final static UUID UUID_ADDE_STATUS_DESCRIPTOR = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

  /** Address of the bluetooth device to monitor */
  String deviceAddress;

  // E-Bike info
  String rawStatus;
  String powerLevel;
  String batteryPercentage;
  String temperature;

  // Operation state
  private final int SCANNING = 0;
  private final int CONNECTING = 1;
  private final int DISCOVERING_SERVICES = 2;
  private final int OPERATING = 3;
  private int state = SCANNING;

  // List of known bluetooth devices
  public static LinkedList<String> knownDeviceAddresses = new LinkedList<String>();

  /** Extracts the heart rate from a characteristics */
  private void updateStatus(BluetoothGattCharacteristic characteristic) {
    String value = characteristic.getStringValue(0);
    if (value.startsWith("<NEXT")) {
      rawStatus=value;
    } else {
      rawStatus+=value;
    }
    if (value.endsWith("\r\n")) {
      //GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDAppEB", "ADD-E status received: " + rawStatus);
      String[] fields=rawStatus.split("\\|");
      if (fields.length>=21) {
        coreObject.configStoreSetStringValue("EBikeMonitor","batteryLevel",fields[2]);
        coreObject.configStoreSetStringValue("EBikeMonitor","powerLevel",fields[3]);
        coreObject.configStoreSetStringValue("EBikeMonitor","engineTemperature",fields[5]);
        coreObject.configStoreSetStringValue("EBikeMonitor","distanceElectric",fields[20]);
        coreObject.configStoreSetStringValue("EBikeMonitor","distanceTotal",fields[19]);
        coreObject.configStoreSetStringValue("EBikeMonitor","speed",fields[12]);
        coreObject.configStoreSetStringValue("EBikeMonitor","connected","1");
        coreObject.executeCoreCommand("dataChanged");
      }
    }
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
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDAppEB", "connection to bluetooth gatt service established, requesting service discovery");
          gatt.discoverServices();
          state = DISCOVERING_SERVICES;
        }
      } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB","connection to bluetooth gatt service dropped");
        state = CONNECTING;
        coreObject.playSound("eBikeDisconnect.wav", 1, 100);
        coreObject.configStoreSetStringValue("EBikeMonitor","connected","0");
        coreObject.executeCoreCommand("dataChanged");
      }
    }

    /** Called when new services are discovered */
    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
      super.onServicesDiscovered(gatt,status);
      if (status == BluetoothGatt.GATT_SUCCESS) {
        if (state == DISCOVERING_SERVICES) {
          state = OPERATING;
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB","bluetooth gatt service discovery completed");
          List<BluetoothGattService> gattServices = gatt.getServices();
          if (gattServices == null) return;
          for (BluetoothGattService gattService : gattServices) {
            GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB",String.format("device supports service %s",gattService.getUuid().toString()));
            if (gattService.getUuid().equals(UUID_ADDE_SERVICE)) {
              List<BluetoothGattCharacteristic> gattCharacteristics = gattService.getCharacteristics();
              if (gattCharacteristics == null) return;
              for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB",String.format("device service supports characteristics %s",gattCharacteristic.getUuid().toString()));
                if (gattCharacteristic.getUuid().equals(UUID_ADDE_STATUS_VALUE)) {
                  GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB",String.format("requesting read of characteristic",gattCharacteristic.getUuid().toString()));
                  gatt.readCharacteristic(gattCharacteristic);
                  gatt.setCharacteristicNotification(gattCharacteristic,true);
                  BluetoothGattDescriptor descriptor = gattCharacteristic.getDescriptor(UUID_ADDE_STATUS_DESCRIPTOR);
                  descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                  gatt.writeDescriptor(descriptor);
                }
              }
            }
          }
          coreObject.playSound("eBikeConnect.wav",1, 100);
        }
      }
    }

    // Result of a characteristic read operation
    @Override
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic,
                                     int status) {
      super.onCharacteristicRead(gatt, characteristic, status);
      if (status == BluetoothGatt.GATT_SUCCESS) {
        if (characteristic.getUuid().equals(UUID_ADDE_STATUS_VALUE)) {
          updateStatus(characteristic);
        }
      }
    }

    // Result of a characteristic update operation
    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
      super.onCharacteristicChanged(gatt, characteristic);
      if (characteristic.getUuid().equals(UUID_ADDE_STATUS_VALUE)) {
        updateStatus(characteristic);
      }
    }
  };

  /** Callback for bluetooth devices discoveries  */
  private ScanCallback scanCallback = new ScanCallback() {

    @Override
    public void onScanResult(int callbackType, ScanResult result) {
      if (knownDeviceAddresses!=null) {
        boolean found=false;
        for (String knownDeviceAddress : knownDeviceAddresses) {
          if (knownDeviceAddress.equals(result.getDevice().getAddress())) {
            found=true;
          }
        }
        if (!found) {
          GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDAppEB", String.format("new bluetooth le device <%s> discovered!",result.getDevice().getAddress()));
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
  static public boolean isSupported(Context context) {
    if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
      return true;
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
  public GDEBikeService(Context context, GDCore coreObject) {

    // Store important references
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppEB","starting service");
    this.context = context;
    this.coreObject = coreObject;
    deviceAddress = coreObject.configStoreGetStringValue("EBikeMonitor", "bluetoothAddress");
    if (!knownDeviceAddresses.contains(deviceAddress))
      knownDeviceAddresses.add(deviceAddress);

    // Setup bluetooth low energy
    bluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
    bluetoothAdapter = bluetoothManager.getAdapter();
    final UUID[] uuids = {UUID_ADDE_SERVICE};
    List<ScanFilter> filterList = createScanFilterList(uuids);
    ScanSettings settings = new ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_BALANCED)
        .build();
    bluetoothScanner = bluetoothAdapter.getBluetoothLeScanner();
    bluetoothScanner.startScan(filterList,settings,scanCallback);
  }

  /** Stops all services */
  public void deinit() {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDAppEB","stopping service");
    if (bluetoothScanner!=null) {
      bluetoothScanner.stopScan(scanCallback);
    }
    if (bluetoothGatt!=null) {
      bluetoothGatt.close();
      bluetoothGatt=null;
    }
  }

}
