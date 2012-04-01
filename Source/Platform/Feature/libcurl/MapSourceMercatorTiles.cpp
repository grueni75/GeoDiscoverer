//============================================================================
// Name        : MapSourceMercatorTiles.cpp
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

#include <curl/curl.h>
#include <Core.h>

namespace GEODISCOVERER {

// Downloads a map image from the server
bool MapSourceMercatorTiles::downloadMapImage(std::string url, std::string filePath) {

  FILE *out;
  char curlErrorBuffer[CURL_ERROR_SIZE];
  std::string tempFilePath = getFolderPath() + "/downloadBuffer.bin";

  // Open the file for writing
  if (!(out=fopen(tempFilePath.c_str(),"w"))) {
    if (!errorOccured) {
      ERROR("can not create map image <%s>",filePath.c_str());
      errorOccured=true;
      return false;
    }
  }

  // Download it with curl
  CURL *curl;
  curl = curl_easy_init();
  if (!curl) {
    FATAL("can not initialize curl library",NULL);
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,curlErrorBuffer);
  DEBUG("downloading %s",url.c_str());
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  // Close the file
  fclose(out);

  // Handle errors
  if (res!=0) {

    // Delete the created file
    remove(filePath.c_str());

    // Output warning
    if (!downloadWarningOccured) {
      WARNING("can not download map image <%s>: %s",url.c_str(),curlErrorBuffer);
      downloadWarningOccured=true;
    }
    return false;

  } else {

    // Move the file to its intended position
    rename(tempFilePath.c_str(),filePath.c_str());
    return true;

  }

}

}


