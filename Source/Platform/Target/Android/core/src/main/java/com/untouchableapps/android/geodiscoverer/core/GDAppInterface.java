//============================================================================
// Name        : GDAppInterface.java
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