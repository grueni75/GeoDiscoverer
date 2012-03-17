//============================================================================
// Name        : Main.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <Core.h>

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
  GEODISCOVERER::core->getScreen()->mainLoop();

  // Exit the thread
  GEODISCOVERER::core->getThread()->exitThread();
}

// Debugging thread
void *debugThread(void *args) {

  // Set an example position
  GEODISCOVERER::core->getCommander()->execute("locationChanged(gps,1289865600000,6.7766815000000005,51.22516803333333,1,100.0,1,1,200.0,1,300.0,1,400.0)");

  // Set an example bearing
  GEODISCOVERER::core->getCommander()->execute("compassBearingChanged(345.3542)");

  // Use the replay log if it exists
  GEODISCOVERER::core->getDebug()->replayTrace("replay.log");

  // Exit the thread
  GEODISCOVERER::core->getThread()->exitThread();
}

// Main routine
int main(int argc, char **argv)
{
  // Create the application
  if (!(GEODISCOVERER::core=new GEODISCOVERER::Core(".",240))) {
    puts("FATAL: can not create geo discoverer core object!");
    exit(1);
  }

  // Start it as a thread
  GEODISCOVERER::ThreadInfo *main=GEODISCOVERER::core->getThread()->createThread(mainThread,NULL);

  // Wait some time
  while (!GEODISCOVERER::core->getIsInitialized()) {
    sleep(1);
  }

  // Start the debugging thread
  GEODISCOVERER::ThreadInfo *debug=GEODISCOVERER::core->getThread()->createThread(debugThread,NULL);

  // Wait until the threads exit
  GEODISCOVERER::core->getThread()->waitForThread(debug);
  GEODISCOVERER::core->getThread()->waitForThread(main);

  // And destruct core object
  GEODISCOVERER::core->getThread()->destroyThread(debug);
  GEODISCOVERER::core->getThread()->destroyThread(main);
  delete GEODISCOVERER::core;
}

