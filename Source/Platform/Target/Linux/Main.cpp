//============================================================================
// Name        : Main.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <Core.h>

// Indicates if the main thread as exited
bool mainThreadHasExited=false;

// Main thread of the application
void *mainThread(void *args) {

  // Init the core object
  if (!GEODISCOVERER::core->init()) {
    puts("FATAL: can not initialize geo discoverer core object!");
    exit(1);
  }

  // Ensure that the widgets are positioned
  GEODISCOVERER::core->getCommander()->execute("screenChanged(landscape,768,480)");

  // Start the main loop
  GEODISCOVERER::core->getDefaultScreen()->mainLoop();

  // Exit the thread
  mainThreadHasExited=true;
  GEODISCOVERER::core->getThread()->exitThread();

  return NULL;
}

// Debugging thread
void *debugThread(void *args) {

  /* Set an example position
  while (!mainThreadHasExited) {
    double bearing = rand() % 359 + 0;
    std::stringstream cmd;
    cmd << "locationChanged(gps,";
    cmd << GEODISCOVERER::core->getClock()->getMicrosecondsSinceStart() << ",";
    double longitude = 6.7766815000000005 + ((double)((rand() % 500) - 250)) / 500.0;
    double latitude = 51.23516803333333 + ((double)((rand() % 500) - 250)) / 500.0;
    cmd << longitude << "," << latitude << ",";
    cmd << "1,100.0,1,";
    cmd << "1," << bearing << ",";
    cmd << "1,4.16,";
    cmd << "1,400.0)";
    GEODISCOVERER::core->getCommander()->execute(cmd.str());
    for (int i=0;i<1;i++) {
      std::stringstream cmd;
      cmd << "setTargetAtGeographicCoordinate(";
      double longitude = 6.7766815000000005 + ((double)((rand() % 500) - 250)) / 500.0;
      double latitude = 51.23516803333333 + ((double)((rand() % 500) - 250)) / 500.0;
      cmd << longitude << "," << latitude << ")";
      GEODISCOVERER::core->getCommander()->execute(cmd.str());
      usleep(rand() % 10000000);
    }
  }*/

  // Set an example position
  //GEODISCOVERER::core->getCommander()->execute("locationChanged(gps,1289865600000,6.7766815000000005,51.23516803333333,1,100.0,1,1,200.0,1,4.16,1,400.0)");
  GEODISCOVERER::core->getCommander()->execute("locationChanged(gps,1289865600000,6.81552,51.238585,1,100.0,1,1,0.0,1,4.16,1,400.0)");

  // Set an example bearing
  GEODISCOVERER::core->getCommander()->execute("compassBearingChanged(345.3542)");

  // Set an example target
  //sleep(5);
  //GEODISCOVERER::core->getCommander()->execute("newPointOfInterest(6.70,51.2)");
  //sleep(5);
  //GEODISCOVERER::core->getCommander()->execute("setTargetAtGeographicCoordinate(6.75,51.0)");

  // Hide the target
  //sleep(5);
  //GEODISCOVERER::core->getCommander()->execute("hideTarget()");

  // Redownload the visible map
  sleep(3);
  GEODISCOVERER::core->getCommander()->execute("forceMapRedownload(1)");

  // Use the replay log if it exists
  GEODISCOVERER::core->getDebug()->replayTrace("replay.log");

  // Exit the thread
  GEODISCOVERER::core->getThread()->exitThread();

  return NULL;
}

// Main routine
int main(int argc, char **argv)
{
  // Create the application
  if (!(GEODISCOVERER::core=new GEODISCOVERER::Core(".",240,5))) {
    puts("FATAL: can not create geo discoverer core object!");
    exit(1);
  }

  // Start it as a thread
  GEODISCOVERER::ThreadInfo *main=GEODISCOVERER::core->getThread()->createThread("main thread",mainThread,NULL);

  // Wait some time
  while ((!mainThreadHasExited)&&(!GEODISCOVERER::core->getIsInitialized())) {
    sleep(1);
  }

  // Start the debugging thread
  GEODISCOVERER::ThreadInfo *debug=NULL;
  if (!mainThreadHasExited)
    debug=GEODISCOVERER::core->getThread()->createThread("debug thread",debugThread,NULL);

  // Wait until the threads exit
  if (debug)
    GEODISCOVERER::core->getThread()->waitForThread(debug);
  GEODISCOVERER::core->getThread()->waitForThread(main);

  // And destruct core object
  if (debug)
    GEODISCOVERER::core->getThread()->destroyThread(debug);
  GEODISCOVERER::core->getThread()->destroyThread(main);
  delete GEODISCOVERER::core;
  GEODISCOVERER::Core::unload();
}

