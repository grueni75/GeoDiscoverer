//============================================================================
// Name        : Donate.java
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


package com.untouchableapps.android.geodiscoverer;

//import org.sufficientlysecure.donations.DonationsFragment;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;

public class Donate extends FragmentActivity {

  // Definitions for google play in app billing
  private static final String GOOGLE_PUBKEY = "?";
  private static final String[] GOOGLE_CATALOG = new String[]{
    "geodiscoverer.donation.1",
    "geodiscoverer.donation.2",
    "geodiscoverer.donation.3",
    "geodiscoverer.donation.5",
    "geodiscoverer.donation.10",
    "geodiscoverer.donation.20"
  };

  /**
   * Called when the activity is first created.
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      setTitle(R.string.support_geodiscoverer);
      setContentView(R.layout.donate);

      /*
      FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
      DonationsFragment donationsFragment;
      donationsFragment = DonationsFragment.newInstance(false, true, GOOGLE_PUBKEY, GOOGLE_CATALOG,
              getResources().getStringArray(R.array.donation_google_catalog_values), false, null, null,
              null, false, null, null, false, null);
      ft.replace(R.id.donations_activity_container, donationsFragment, "donationsFragment");
      ft.commit();*/
  }

  /**
   * Needed for Google Play In-app Billing. It uses startIntentSenderForResult(). The result is not propagated to
   * the Fragment like in startActivityForResult(). Thus we need to propagate manually to our Fragment.
   *
   * @param requestCode
   * @param resultCode
   * @param data
   */
  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);

      FragmentManager fragmentManager = getSupportFragmentManager();
      Fragment fragment = fragmentManager.findFragmentByTag("donationsFragment");
      if (fragment != null) {
          fragment.onActivityResult(requestCode, resultCode, data);
      }
  }

}
