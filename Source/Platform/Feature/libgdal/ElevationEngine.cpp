//============================================================================
// Name        : ElevationEngine.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

#include <Core.h>
#include <ElevationEngine.h>
#include <gdal_utils.h>
#include <Image.h>
#include <ProfileEngine.h>

namespace GEODISCOVERER {

// Initialzes the engine
bool ElevationEngine::init() {

  // Init GDAL
  GDALAllRegister();

  // Reserve memories for the arrays
  if (!(demDatasetBusy=(bool*)malloc((workerCount+1)*sizeof(demDatasetBusy)))) {
    FATAL("no memory",NULL);
    return false;
  }
  memset(demDatasetBusy,0,(workerCount+1)*sizeof(demDatasetFullRes));
  if (!(demDatasetFullRes=(DEMDataset**)malloc((workerCount+1)*sizeof(demDatasetFullRes)))) {
    FATAL("no memory",NULL);
    return false;
  }
  memset(demDatasetFullRes,0,(workerCount+1)*sizeof(demDatasetFullRes));
  if (!(demDatasetLowRes=(DEMDataset**)malloc(workerCount*sizeof(demDatasetLowRes)))) {
    FATAL("no memory",NULL);
    return false;
  }
  memset(demDatasetLowRes,0,workerCount*sizeof(demDatasetLowRes));

  // Configure GDAL
  CPLSetConfigOption("GDAL_CACHEMAX", core->getConfigStore()->getStringValue("General","gdalMaxCacheSize",__FILE__,__LINE__).c_str());

  // Try to open the DEM data
  std::string demFilePathFullRes=demFolderPath+"/index.vrt";
  std::string demFilePathLowRes=demFolderPath+"/lowres.tif";
  for (Int i=0;i<workerCount;i++) {
    demDatasetLowRes[i] = (DEMDataset *) GDALOpen(demFilePathLowRes.c_str(), GA_ReadOnly);
    if (demDatasetFullRes==NULL) {
      WARNING("please install low resolution elevation data into <%s>",demFilePathLowRes.c_str());
      return true;
    }
  }
  for (Int i=0;i<workerCount+1;i++) {
    demDatasetFullRes[i] = (DEMDataset *) GDALOpen(demFilePathFullRes.c_str(), GA_ReadOnly);
    if (demDatasetFullRes==NULL) {
      WARNING("please install full resolution elevation data into <%s>",demFilePathFullRes.c_str());
      return true;
    }
  }

  // Setup the source SRS for WGS84
  CPLErrorReset();
  OGRSpatialReferenceH hSRS = OSRNewSpatialReference( nullptr );
  pszSourceSRS = nullptr;
  if( OSRSetFromUserInput( hSRS, "WGS84" ) == OGRERR_NONE ) {
    OSRExportToWkt( hSRS, &pszSourceSRS );
  } else {
    FATAL("source SRS cannot be set",NULL);
    return false;
  }
  OSRDestroySpatialReference( hSRS );

  // Setup coordinate transformation
  hSrcSRS = OSRNewSpatialReference( pszSourceSRS );
  OSRSetAxisMappingStrategy(hSrcSRS, OAMS_TRADITIONAL_GIS_ORDER);
  auto hTrgSRS = GDALGetSpatialRef( demDatasetFullRes[workerCount] );
  if (!hTrgSRS) {
    FATAL("cannot create spatial reference ",NULL);
    return false;
  }
  hCT = OCTNewCoordinateTransformation( hSrcSRS, hTrgSRS );
  if (!hCT) {
    FATAL("cannot create coordinate transformator",NULL);
    return false;
  }
  if (GDALGetGeoTransform(demDatasetFullRes[workerCount], adfGeoTransform) != CE_None) {
    FATAL("cannot create geo transformation",NULL);
    return false;
  }
  if (!GDALInvGeoTransform(adfGeoTransform, adfInvGeoTransform)) {
    FATAL("cannot create inverse geo transformation",NULL);
    return false;
  }
  hBand = GDALGetRasterBand( demDatasetFullRes[workerCount], 1);
  if (!hBand) {
    FATAL("cannot find hBand in DEM dataset",NULL);
    return false;
  }
  
  // We are ready
  isInitialized=true;

  /* Debugging
  MapPosition pos;
  pos.setLng(9.496885);
  pos.setLat(46.797145);
  // http://stefanosabatini.eu/tools/tilecalc/
  // 10/46.797145/9.496885 => 10/539/361
  //renderHillshadeTile(5,11,16,demFolderPath+"/test5.png");
  PROFILE_START;
  renderHillshadeTile(10,361,539,demFolderPath+"/test10.png");
  PROFILE_ADD("zoom level 10");
  renderHillshadeTile(17,46209,68993,demFolderPath+"/test17.png");
  PROFILE_ADD("zoom level 17");
  PROFILE_END;
  //FATAL("stop here",NULL);*/

  return true;
}

// Clears the engine
void ElevationEngine::deinit() {

  // Interrupt the threads accessing this object
  //DEBUG("waiting for all threads to finish",NULL);
  isInitialized=false;
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  if (demDatasetBusy) {
    //DEBUG("requesting all render threads to quit",NULL);
    while (true) {
      bool threadActive=false;
      for (int i=0;i<workerCount;i++) {
        if (demDatasetBusy[i]) {
          threadActive=true;
          break;
        }
      }
      if (!threadActive) {
        break;
      } else {
        //DEBUG("waiting for next thread finishing",NULL);
        core->getThread()->unlockMutex(accessMutex);
        core->getThread()->waitForSignal(demDatasetReadySignal);
        core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      }
    }
    //DEBUG("no thread active anymore",NULL);

    // Wait for any thread getting alitutde information
    //DEBUG("waiting for thread using the DEM dataset for altitude calculation",NULL);
    while (demDatasetBusy[workerCount]) {
      core->getThread()->unlockMutex(accessMutex);
      usleep(1000);
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    }
  }  

  // Close the stuff needed for coordinate transformation
  if (hSrcSRS) {
    OSRDestroySpatialReference(hSrcSRS);
    hSrcSRS=NULL;
  }
  if (hCT) {
    OCTDestroyCoordinateTransformation(hCT);
    hCT=NULL;
  }
  // Blocks under Android for unknown reasons
  if (pszSourceSRS) {
    CPLFree(pszSourceSRS);
    pszSourceSRS=NULL;
  }

  // Close the dem data
  if (demDatasetFullRes) {
    for (Int i=0;i<workerCount+1;i++) {
      GDALClose(demDatasetFullRes[i]);
    }
    free(demDatasetFullRes);
    demDatasetFullRes=NULL;
  }
  if (demDatasetLowRes) {
    for (Int i=0;i<workerCount;i++) {
      GDALClose(demDatasetLowRes[i]);
    }
    free(demDatasetLowRes);
    demDatasetLowRes=NULL;
  }
  if (demDatasetBusy) {
    free(demDatasetBusy);
    demDatasetBusy=NULL;
  }
}

// Converts a tile y number into latitude
double ElevationEngine::tiley2lat(Int y, Int worldRes) {
  double n = M_PI-2.0*M_PI*((double)y)/((double)worldRes);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-1*n)));
}

// Converts a tile x number into longitude
double ElevationEngine::tilex2lon(Int x, Int worldRes) {
  return ((double)x) / ((double)worldRes) * 360.0 - 180;
}

// Resets dem dataset busy indicator for the given worker number
void ElevationEngine::resetDemDatasetBusy(Int workerNr) {
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  demDatasetBusy[workerNr]=false;
  core->getThread()->unlockMutex(accessMutex);
  core->getThread()->issueSignal(demDatasetReadySignal);
}

// Creates a hillshading for the given map area
UByte *ElevationEngine::renderHillshadeTile(Int z, Int y, Int x, UInt &imageSize) {

  // Do not work if not initialized anymore
  if (!isInitialized)
    return NULL;

  // Get the dataset to use
  DEMDataset *demDatasetFullRes;
  DEMDataset *demDatasetLowRes;
  Int workerNr;
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  while (true) {
    bool found=false;
    for (int i=0;i<workerCount;i++) {
      if (!demDatasetBusy[i]) {
        //DEBUG("using dem dataset %d",i);
        demDatasetFullRes=this->demDatasetFullRes[i];
        demDatasetLowRes=this->demDatasetLowRes[i];
        demDatasetBusy[i]=true;
        workerNr=i;
        found=true;
        break;
      }
    }
    if (found) {
      break;
    } else {
      //DEBUG("waiting for next available dem dataset",NULL);
      core->getThread()->unlockMutex(accessMutex);
      core->getThread()->waitForSignal(demDatasetReadySignal);
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    }
  }

  // Decide on the filenames
  std::stringstream imageFilenameStream;
  imageFilenameStream << "/vsimem/hillshade_" << tileNumber << "_" << z << "_" << y << "_" << x <<".png";
  std::string imageFilename=imageFilenameStream.str();
  std::stringstream warpFilename;
  warpFilename << "/vsimem/warp_" << tileNumber << "_" << z << "_" << y << "_" << x << ".tif";
  tileNumber++;
  core->getThread()->unlockMutex(accessMutex);

  // Calculate the bounding box
  double worldRes = pow(2.0, z);
  double lonRes = 360.0 / worldRes;
  double lon1 = tilex2lon(std::max(0, x-1), worldRes);
  double lat1 = tiley2lat(std::max(0, y-1), worldRes);
  double lat2 = tiley2lat(std::min(worldRes, (double)y+2), worldRes);
  double lon2 = tilex2lon(std::min(worldRes, (double)x+2), worldRes);
  double latMin = std::min(lat1, lat2);
  double lonMin = std::min(lon1, lon2);
  double latMax = std::max(lat1, lat2);
  double lonMax = std::max(lon1, lon2);
  Int imageWidth=256;
  Int imageHeight=256;
  Int renderWidth=imageWidth;
  Int renderHeight=imageHeight;
  Int cropX1=0;
  Int cropY1=0;
  if (x>0) {
    cropX1=256;
    renderWidth+=256;
  }  
  if (y>0) {
    cropY1=256;
    renderHeight+=256;
  }
  Int cropX2=renderWidth;
  Int cropY2=renderHeight;
  if (x<worldRes-1) {
    renderWidth+=256;
    cropX2=renderWidth-256;
  }
  if (y<worldRes-1) {
    renderHeight+=256;
    cropY2=renderHeight-256;
  }

  // Convert variables to strings
  std::stringstream lonMinStr,latMinStr,lonMaxStr,latMaxStr,widthStr,heightStr;
  lonMinStr<<lonMin;
  lonMaxStr<<lonMax;
  latMinStr<<latMin;
  latMaxStr<<latMax;
  widthStr<<renderWidth;
  heightStr<<renderHeight;

  // Create the options for warping into the correct projection
  const char *interpolationAlgo="cubic";
  /*if (z<11)
    interpolationAlgo="near";*/
  const char *warpArgs[] = {
    "-t_srs",
    "EPSG:3857",
    "-r",
    interpolationAlgo,
    "-te",
    lonMinStr.str().c_str(),
    latMinStr.str().c_str(),
    lonMaxStr.str().c_str(),
    latMaxStr.str().c_str(),
    "-te_srs",
    "EPSG:4326",
    "-ts",
    widthStr.str().c_str(),
    heightStr.str().c_str(),
    NULL
  };
  GDALWarpAppOptions *warpOptions = GDALWarpAppOptionsNew((char**)warpArgs,NULL);
  if (warpOptions==NULL) {
    FATAL("can not create warp options",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }

  // Decide which data source to use
  GDALDataset *srcDS[] = { demDatasetFullRes };
  if (z<=lowResZoomLevel)
    srcDS[0]=demDatasetLowRes;
  
  // Warp the source data set into the correct projection
  int usageError = FALSE;
  GDALDatasetH warpDS = GDALWarp(warpFilename.str().c_str(), NULL, 1, (GDALDatasetH*)srcDS, warpOptions, &usageError);
  GDALWarpAppOptionsFree(warpOptions);
  if ((usageError)||(warpDS==NULL)) {
    DEBUG("gdal warp operation failed",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }

  // Create the options for the hillshading
  const char *alg;
  if (z<11) 
    alg="Horn";
  else
    alg="ZevenbergenThorne";
  const char *hillshadeArgs[] = {
    "-z",
    "2",
    "-az",
    "315",
    "-alg",
    alg,
    NULL
  };
  GDALDEMProcessingOptions *hillshadeOptions = GDALDEMProcessingOptionsNew((char**)hillshadeArgs,NULL);
  if (hillshadeOptions==NULL) {
    FATAL("can not create hillshade options",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }

  // Render the hillshade
  GDALDatasetH hillshadeDS=GDALDEMProcessing(imageFilename.c_str(),warpDS,"hillshade",NULL,hillshadeOptions,&usageError);
  GDALDEMProcessingOptionsFree(hillshadeOptions);
  GDALClose(warpDS);
  VSIUnlink(warpFilename.str().c_str());
  if ((usageError)||(hillshadeDS==NULL)) {
    DEBUG("gdal hillshade operation failed",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }
  GDALClose(hillshadeDS);

  // Load the png image
  vsi_l_offset size;
  GByte *data=VSIGetMemFileBuffer(imageFilename.c_str(), &size, true);  
  UInt pixelSize;
  ImagePixel *hillshadePixels=core->getImage()->loadPNG((UByte*)data,size,renderWidth,renderHeight,pixelSize,false);
  VSIFree(data);
  VSIUnlink((imageFilename+".aux.xml").c_str());
  if ((!hillshadePixels)||(pixelSize!=Image::getRGBPixelSize())) {
    FATAL("can not read <%s>",imageFilename.c_str());
    resetDemDatasetBusy(workerNr);
    return NULL;
  }
  //DEBUG("renderWidth=%d renderHeight=%d pixelSize=%d",renderWidth,renderHeight,pixelSize);

  // Copy this one in a RGBA image with alpha only
  // Invert the pixels on the run
  ImagePixel *filterPixels=(ImagePixel*)malloc(renderWidth*renderHeight);
  if (!filterPixels) {
    FATAL("can not reserve memory",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }
  for (Int i=0;i<renderWidth*renderHeight;i++) {

    // Invert the original pixels
    // Replace black pixels with pure transparent and lighten the pixels slightly
    ImagePixel p=255-hillshadePixels[i*Image::getRGBPixelSize()];
    if ((p==255)||(p<=74))
      filterPixels[i]=0;
    else 
      filterPixels[i]=p-74;
  }
  free(hillshadePixels);

  // Now apply a blur filter
  double blurRadius=z-10;
  if (z>=17) 
    blurRadius=blurRadius*1.5;
  //DEBUG("blurRadius=%f",blurRadius);
  if (blurRadius>1) {
    core->getImage()->iirGaussFilter(filterPixels,renderWidth,renderHeight,1,3*blurRadius);
  }

  // Crop the image
  ImagePixel *imagePixels=(ImagePixel*)malloc(imageWidth*imageHeight*Image::getRGBAPixelSize());
  if (!imagePixels) {
    FATAL("can not reserve memory",NULL);
    resetDemDatasetBusy(workerNr);
    return NULL;
  }
  Int t=0;
  //DEBUG("cropX1=%d cropX2=%d cropY1=%d cropY2=%d",cropX1,cropX2,cropY1,cropY2);
  for (int y=cropY1;y<cropY2;y++) {
    for (int x=cropX1;x<cropX2;x++) {
      imagePixels[t*Image::getRGBAPixelSize()+0]=0;
      imagePixels[t*Image::getRGBAPixelSize()+1]=0;
      imagePixels[t*Image::getRGBAPixelSize()+2]=0;
      imagePixels[t*Image::getRGBAPixelSize()+3]=filterPixels[y*renderWidth+x];
      t++;
    }
  }
  free(filterPixels);
  //DEBUG("t=%d",t);

  // Store the final image
  imageSize=0;
  UByte *pngPixels=core->getImage()->writePNG(imagePixels,imageWidth,imageHeight,Image::getRGBAPixelSize(),imageSize,false);
  free(imagePixels);

  // Clean up
  //core->getThread()->unlockMutex(accessMutex);
  resetDemDatasetBusy(workerNr);
  return pngPixels;
}

// Returns the altitude of the given position
bool ElevationEngine::getElevation(MapPosition *pos) {

  // Only work if initialized
  pos->setHasAltitude(false);
  if (!isInitialized) {
    DEBUG("DEM dataset not yet initialized",NULL);
    return false;
  }
  while (demDatasetBusy[workerCount]) {
    DEBUG("waiting for DEM dataset availability",NULL);
    usleep(1000);
  }

  // Indicate that we are working
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  demDatasetBusy[workerCount]=true;
  core->getThread()->unlockMutex(accessMutex);

  // Turn the location into a pixel and line location
  double lng=pos->getLng();
  double lat=pos->getLat();
  int iPixel, iLine;
  if (hCT) {
    if (!OCTTransform(hCT, 1, &lng, &lat, nullptr)) {
      DEBUG("cannot transform coordinates (%f,%f)",lng,lat);
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      demDatasetBusy[workerCount]=false;
      core->getThread()->unlockMutex(accessMutex);
      return false;
    }
  }
  iPixel = static_cast<int>(floor(
    adfInvGeoTransform[0]
    + adfInvGeoTransform[1] * lng
    + adfInvGeoTransform[2] * lat));
  iLine = static_cast<int>(floor(
    adfInvGeoTransform[3]
    + adfInvGeoTransform[4] * lng
    + adfInvGeoTransform[5] * lat));
  if(iPixel < 0 || iLine < 0
     || iPixel >= GDALGetRasterXSize(demDatasetFullRes[workerCount])
     || iLine  >= GDALGetRasterYSize(demDatasetFullRes[workerCount])) {
    DEBUG("coordinate (%f,%f) is outside the DEM dataset",pos->getLng(),pos->getLat());
    core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    demDatasetBusy[workerCount]=false;
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Query the altitude from the DEM dataset
  double adfPixel[2];
  bool result=false;
  if (GDALRasterIO(hBand, GF_Read, iPixel, iLine, 1, 1, adfPixel, 1, 1, GDT_CFloat64, 0, 0) == CE_None) {
    if (adfPixel[0]!=-32768) {
      pos->setAltitude(adfPixel[0]);
      pos->setHasAltitude(true);
      //pos->setIsWGS84Altitude(true);
      //pos->toMSLHeight();
      //DEBUG("MSL altitude at (%f,%f) is %f",pos->getLat(),pos->getLng(),pos->getAltitude());
      result=true;
    }
  }
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  demDatasetBusy[workerCount]=false;
  core->getThread()->unlockMutex(accessMutex);

  return result;
}

}
