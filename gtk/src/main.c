/* main.c
 *
 * Copyright (C) 2008-2026 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "application.h"
#include "settings.h"
#include "ui_res.h"

static char *
get_locale_dir (const char *app_dir)
{
    g_autofree char *prefix = g_path_get_dirname(app_dir);
    return g_canonicalize_filename(PACKAGE_LOCALE_DIR, prefix);
}

static char *
get_app_dir (const char *app_cmd)
{
    // The preferred method, only works on Linux and certain other OSes
    g_autofree char *link = g_file_read_link("/proc/self/exe", NULL);
    if (link != NULL)
        return g_path_get_dirname(link);

    // Alternatively, work out the path from the command name, path and cwd
    if (g_path_is_absolute(app_cmd))
        return g_path_get_dirname(app_cmd);

    g_autofree char *path_cmd = g_find_program_in_path(app_cmd);
    if (path_cmd != NULL)
        return g_path_get_dirname(path_cmd);

    g_autofree char *cwd = g_get_current_dir();
    g_autofree char *cmd_dir = g_path_get_dirname(app_cmd);
    return g_canonicalize_filename(app_cmd, cwd);
}

int
main (int argc, char *argv[])
{
#if defined(_WIN32)
    // Tell gdk pixbuf where it's loader config file is.
    _putenv_s("GDK_PIXBUF_MODULE_FILE", "ghb.exe.local/loaders.cache");
#endif
    g_autofree char *app_dir = get_app_dir(argv ? argv[0] : NULL);
    const char *textdomaindir = getenv("TEXTDOMAINDIR");
    g_autofree char *locale_dir;
    if (textdomaindir)
        locale_dir = g_strdup(textdomaindir);
    else
        locale_dir = get_locale_dir(app_dir); 
    bindtextdomain(GETTEXT_PACKAGE, locale_dir);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    g_autoptr(GApplication) app = G_APPLICATION(ghb_application_new(app_dir));
    return g_application_run(app, argc, argv);
}
