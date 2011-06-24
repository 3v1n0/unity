/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "gtkloader.h"

COMPIZ_PLUGIN_20090315 (gtkloader, GTKLoaderPluginVTable);

GTKLoaderScreen::GTKLoaderScreen (CompScreen *screen) :
    PluginClassHandler <GTKLoaderScreen, CompScreen> (screen)
{
}

bool
GTKLoaderPluginVTable::init ()
{
    int (*old_handler) (Display *, XErrorEvent *);
    old_handler = XSetErrorHandler (NULL);

    XSetErrorHandler (old_handler);

    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    if (!gtk_init_check (&programArgc, &programArgv))
    {
	compLogMessage ("unitydialog", CompLogLevelError, "Couldn't initialize gtk");
	return false;
    }

    XSetErrorHandler (old_handler);
}
