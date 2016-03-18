//============================================================================
// Name        : GDAppInterface.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer.core;

import android.app.Application;
import android.content.Context;
import android.content.Intent;

/** Interface used by the cockpit engine on the cockpit apps */
public interface GDAppInterface {

  // Severity levels for messages
  int ERROR_MSG = 0;
  int WARNING_MSG = 1;
  int INFO_MSG = 2;
  int DEBUG_MSG = 3;
  int FATAL_MSG = 4;

  /**
   * Adds a message to the log
   */
  void addAppMessage(int severityNumber, String tag, String message);

  /**
   * Starts the cockpit engine
   */
  void cockpitEngineStart();

  /**
   * Updates the cockpit engine with the new infos
   */
  void cockputEngineUpdate(String infos);

  /**
   * Stops the cockpit engine
   */
  void cockpitEngineStop();

  /**
   * Checks if the cockpit engine is active()
   */
  boolean cockpitEngineIsActive();

  /**
   * Sends a native crash report
   */
  void sendNativeCrashReport(String dumpBinPath, boolean quitApp);

  /**
   * Returns the application context
   */
  Context getContext();

  /**
   * Sends a command to the activity
   */
  void executeAppCommand(String cmd);

  /**
   * Returns the orientation of the activity
   */
  int getActivityOrientation();

  /**
   * Returns an intent for the GDService
   */
  Intent createServiceIntent();

  /**
   * Returns the application
   */
  Application getApplication();

  /**
   * Sends a command to the wear device
   */
  void sendWearCommand( final String command );

}