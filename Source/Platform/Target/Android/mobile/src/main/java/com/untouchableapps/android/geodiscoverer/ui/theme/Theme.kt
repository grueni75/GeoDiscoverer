//============================================================================
// Name        : Theme.kt
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

package com.untouchableapps.android.geodiscoverer.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material.ContentAlpha
import androidx.compose.material3.ColorScheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.compositeOver
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.untouchableapps.android.geodiscoverer.core.R
import kotlin.math.ln

@Composable
fun Material2Theme(content: @Composable () -> Unit) {
  val darkColorScheme = androidx.compose.material.darkColors(
    primary = colorResource(id = R.color.gd_orange),
  )
  androidx.compose.material.MaterialTheme(
    colors = darkColorScheme,
    typography = M2Typography,
    content = content
  )
}

@Composable
fun AndroidTheme(content: @Composable() () -> Unit) {
  val darkColorScheme = darkColorScheme(
    primary = colorResource(id = R.color.gd_orange),
    surface = colorResource(id = R.color.gd_surface),
    onPrimary = darkColorScheme().onBackground,
    primaryContainer = colorResource(id = R.color.gd_background_orange)
  )
  MaterialTheme(
    colorScheme = darkColorScheme,
    typography = M3Typography,
    content = content
  )
}

// Copy from ColorScheme.surfaceColorAtElevation (as it is private)
@Composable
fun SurfaceColorAtElevation(
  elevation: Dp,
): Color {
  if (elevation == 0.dp) return MaterialTheme.colorScheme.surface
  val alpha = ((4.5f * ln(elevation.value + 1)) + 2f) / 100f
  return MaterialTheme.colorScheme.primary.copy(alpha = alpha).compositeOver(MaterialTheme.colorScheme.surface)
}
