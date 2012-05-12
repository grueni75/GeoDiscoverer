//============================================================================
// Name        : Core.cpp
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

// Downloads a URL
bool Core::downloadURL(std::string url, std::string filePath, bool generateMessages) {

  FILE *out;
  char curlErrorBuffer[CURL_ERROR_SIZE];

  // Open the file for writing
  if (!(out=fopen(filePath.c_str(),"w"))) {
    if (generateMessages) {
      ERROR("can not create file <%s>",filePath.c_str());
    }
    return false;
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
  DEBUG("downloading url %s",url.c_str());
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  // Close the file
  fclose(out);

  // Handle errors
  if (res!=0) {

    // Delete the created file
    remove(filePath.c_str());

    // Output warning
    if (generateMessages) {
      WARNING("can not download url <%s>: %s",url.c_str(),curlErrorBuffer);
    }
    return false;

  } else {
    return true;
  }
}

}





