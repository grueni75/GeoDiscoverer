package com.untouchableapps.android.geodiscoverer.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.res.colorResource
import com.untouchableapps.android.geodiscoverer.core.R

@Composable
fun AndroidTheme(content: @Composable() () -> Unit) {
    val darkColorScheme = darkColorScheme(
        primary = colorResource(id = R.color.gd_orange),
        surface = colorResource(id = R.color.gd_surface)
    )
    MaterialTheme(
        colorScheme = darkColorScheme,
        typography = Typography,
        content = content
    )
}