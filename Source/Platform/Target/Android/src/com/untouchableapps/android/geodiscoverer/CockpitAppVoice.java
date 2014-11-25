//============================================================================
// Name        : CockpitAppVoice.java
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

package com.untouchableapps.android.geodiscoverer;

import java.util.Calendar;
import java.util.Locale;

import android.content.Context;
import android.media.MediaPlayer;
import android.speech.tts.TextToSpeech;

public class CockpitAppVoice implements CockpitAppInterface, TextToSpeech.OnInitListener {
  
  // Context
  Context context;
  
  // Last time a navigation alert was spoken
  long lastAlert;
  long minDurationBetweenOffRouteAlerts;
  
  // Text to speech engine for saying something
  TextToSpeech textToSpeech;
  Locale textToSpeechLocale;
  boolean textToSpeechReady = false;
  
  // Current text to say
  String navigationInstructions = null; 
    
  /** Constructor */
  public CockpitAppVoice(Context context) {
    super();
        
    // Init parameters
    minDurationBetweenOffRouteAlerts = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "minDurationBetweenOffRouteAlerts")) *  1000;

    // Prepare the text to speech engine
    textToSpeech = new TextToSpeech(context, this);
    
    // Remember context
    this.context = context;
  }

  /** Prepare voice output */
  public void start() {
    
  }

  /** Not necessary */
  public void focus() {
  }
  
  /** Tell what is happening */
  public void inform() {
  }  

  /** Not necessary */
  public void alert(AlertType type) {
    if (!textToSpeechReady)
      return;
    long t = Calendar.getInstance().getTimeInMillis();
    if (type==AlertType.offRoute) {
      
      // Only speak off route alert with defined distance
      long diffToLastUpdate = t - lastAlert;
      if (diffToLastUpdate>minDurationBetweenOffRouteAlerts) {
        textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
        textToSpeech.speak(context.getString(R.string.tts_off_route), TextToSpeech.QUEUE_ADD, null);
        lastAlert=t;
      }
    } else {
      textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
      textToSpeech.speak(navigationInstructions, TextToSpeech.QUEUE_ADD, null);      
      lastAlert=t;
    }
  }  
  
  /** Prepare the text to say */
  public void update(CockpitInfos infos) {
    
    navigationInstructions="";
    
    // Is a turn coming?
    if (!infos.turnAngle.equals("-")) {
      Float turnAngle = Float.valueOf(infos.turnAngle);
      float distanceNumber = Float.valueOf(infos.turnDistance.substring(0,infos.turnDistance.indexOf(" ")));
      distanceNumber=Math.round(distanceNumber);
      String distanceUnit = infos.turnDistance.substring(infos.turnDistance.indexOf(" ")+1);
      if (distanceUnit.equals("m")) {
        distanceUnit=context.getString(R.string.tts_meters);
      }
      if (distanceUnit.equals("km")) {
        distanceUnit=context.getString(R.string.tts_kilometers);
      }
      if (distanceUnit.equals("Mm")) {
        distanceUnit=context.getString(R.string.tts_megameters);
      }
      if (distanceUnit.equals("mi")) {
        distanceUnit=context.getString(R.string.tts_miles);
      }
      if (distanceUnit.equals("yd")) {
        distanceUnit=context.getString(R.string.tts_yards);
      }
      String distance = String.format(textToSpeechLocale, "%.0f %s", distanceNumber,distanceUnit);
      if (turnAngle>0)
        navigationInstructions=context.getString(R.string.tts_turn_left, distance);
      else
        navigationInstructions=context.getString(R.string.tts_turn_right, distance);
      
    }
  }
  
  /** Clean up everything */
  public void stop() {
    textToSpeech.stop();
    navigationInstructions=null;
  }
  
  /** Called by the text to speech engine if init is complete */
  public void onInit(int status) {
    if (status == TextToSpeech.SUCCESS) {
      textToSpeechLocale = new Locale(context.getString(R.string.tts_locale));
      int result = textToSpeech.setLanguage(textToSpeechLocale);
      if ((result == TextToSpeech.LANG_MISSING_DATA) || (result == TextToSpeech.LANG_NOT_SUPPORTED)) {
        GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_set_language) + "\")");
        return;
      }
      if (textToSpeech.addEarcon("[alert]", context.getPackageName(), R.raw.alert)!=TextToSpeech.SUCCESS) {
        GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_add_alert_earcon) + "\")");
        return;
      }
      textToSpeechReady=true;
    } else {
      GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_init) + "\")");
    }    
  }

}
