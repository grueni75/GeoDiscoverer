//============================================================================
// Name        : Theme
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2022 Matthias Gruenewald
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
package com.untouchableapps.android.geodiscoverer.theme

import androidx.compose.runtime.Composable
import androidx.compose.ui.res.colorResource
import androidx.wear.compose.material.Colors
import androidx.wear.compose.material.MaterialTheme
import com.untouchableapps.android.geodiscoverer.core.R

@Composable
fun WearAppTheme(
    content: @Composable () -> Unit
) {
    val wearColorPalette = Colors(
        primary = colorResource(id = R.color.gd_orange),
        surface = colorResource(id = R.color.gd_surface),
    )
    MaterialTheme(
        colors = wearColorPalette,
        typography = Typography,
        // For shapes, we generally recommend using the default Material Wear shapes which are
        // optimized for round and non-round devices.
        content = content
    )
}
