// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

/*
 * These are the precompiled header includes for this module.
 * Only system header files can be listed here.
 */

#include <atk/atk.h>
#include <glib.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <stdio.h>
#include <atk-bridge.h>
#include <Nux/Nux.h>
#include <Nux/WindowThread.h>
#include <NuxCore/Object.h>
