package com.untouchableapps.android.geodiscoverer.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.res.colorResource
import com.untouchableapps.android.geodiscoverer.core.R

@Composable
fun Material2Theme(content: @Composable () -> Unit) {
    val darkColorScheme = androidx.compose.material.darkColors(
        primary = colorResource(id = R.color.gd_orange)
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
        surface = colorResource(id = R.color.gd_surface)
    )
    MaterialTheme(
        colorScheme = darkColorScheme,
        typography = M3Typography,
        content = content
    )
}