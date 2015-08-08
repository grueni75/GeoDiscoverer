//============================================================================
// Name        : Core.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <curl/curl.h>
#include <Core.h>

namespace GEODISCOVERER {

// Downloads a URL
DownloadResult Core::downloadURL(std::string url, std::string filePath, bool generateMessages, bool ignoreFileNotFoundErrors) {

  FILE *out;
  char curlErrorBuffer[CURL_ERROR_SIZE];
  DownloadResult result = DownloadResultOtherFail;
  long curlResponseCode = 0;

  // Open the file for writing
  if (!(out=fopen(filePath.c_str(),"w"))) {
    if (generateMessages) {
      ERROR("can not create file <%s>",filePath.c_str());
    }
    return DownloadResultOtherFail;
  }

  // Download it with curl
  CURL *curl;
  curl = curl_easy_init();
  if (!curl) {
    FATAL("can not initialize curl library",NULL);
    return DownloadResultOtherFail;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Geo Discoverer (build on "  __DATE__  ")");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,curlErrorBuffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  //DEBUG("downloading url %s",url.c_str());
  CURLcode curlResult = curl_easy_perform(curl);
  curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &curlResponseCode);
  curl_easy_cleanup(curl);

  // Close the file
  fclose(out);

  // Handle errors
  switch(curlResult) {
    case CURLE_REMOTE_FILE_NOT_FOUND:
      result=DownloadResultFileNotFound;
      break;
    case CURLE_OK:
      switch (curlResponseCode) {
        case 200:
          result=DownloadResultSuccess;
          break;
        case 404:
          result=DownloadResultFileNotFound;
          break;
        default:
          result=DownloadResultOtherFail;
          break;
      }
      snprintf(curlErrorBuffer,CURL_ERROR_SIZE,"response code = %ld",curlResponseCode);
      break;
    default:
      result=DownloadResultOtherFail;
      break;
  }
  if (result!=DownloadResultSuccess) {

    // Delete the created file
    remove(filePath.c_str());

    // Output warning
    if (generateMessages) {
      bool outputWarning=true;
      if ((ignoreFileNotFoundErrors)&&(result=DownloadResultFileNotFound))
        outputWarning=false;
      if (outputWarning) {
        WARNING("can not download url <%s>: %s",url.c_str(),curlErrorBuffer);
      } else {
        DEBUG("can not download url <%s>: %s",url.c_str(),curlErrorBuffer);
      }
    }
  }
  return result;
}

}





