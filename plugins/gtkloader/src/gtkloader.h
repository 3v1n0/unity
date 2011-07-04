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

#include <gtk/gtk.h>
#include <core/core.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <core/pluginclasshandler.h>

#include "gtkloader_options.h"

class GTKLoaderScreen :
    public PluginClassHandler <GTKLoaderScreen, CompScreen>,
    public GtkloaderOptions
{
    public:

	GTKLoaderScreen (CompScreen *);
};

class GTKLoaderPluginVTable :
    public CompPlugin::VTableForScreen <GTKLoaderScreen>
{
    public:

	bool init ();
};
