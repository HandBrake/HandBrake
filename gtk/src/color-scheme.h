/* Copyright (C) 2022-2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H

#include <gtk/gtk.h>
#include <glib.h>

G_BEGIN_DECLS

/*
 * An enum representing the color state requested by
 * the application. Before the color-scheme library is
 * set up, the application is effectively in the
 * APP_FORCES_LIGHT state.
 */
typedef enum {
    APP_PREFERS_LIGHT,
    APP_PREFERS_DARK,
    APP_FORCES_LIGHT,
    APP_FORCES_DARK
} AppColorScheme;

/*
 * An enum representing the color state given by the
 * desktop portal. If the portal is not available, it
 * defaults to DESKTOP_NO_PREFERENCE.
 */
typedef enum {
    DESKTOP_NO_PREFERENCE,
    DESKTOP_PREFERS_DARK,
    DESKTOP_PREFERS_LIGHT
} DesktopColorScheme;

gboolean color_scheme_set (AppColorScheme scheme);
void color_scheme_set_async (AppColorScheme scheme);
gboolean color_scheme_is_dark_theme (void);
gboolean color_scheme_toggle (void);
AppColorScheme color_scheme_get_app_scheme (void);
DesktopColorScheme color_scheme_get_desktop_scheme (void);

G_END_DECLS

#endif /* COLOR_SCHEME_H */
