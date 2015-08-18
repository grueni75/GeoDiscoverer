//============================================================================
// Name        : Donate.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
