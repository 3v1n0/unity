// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_HOME_VIEW_H
#define PLACES_HOME_VIEW_H

#include <Nux/View.h>
#include <Nux/Layout.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

#include <Nux/GridHLayout.h>

#include "PlacesTile.h"
#include "PlacesGroup.h"

#include <gconf/gconf-client.h>
#include <time.h>

namespace unity
{

class PlacesHomeView : public unity::Introspectable, public PlacesGroup
{
public:
  PlacesHomeView();
  ~PlacesHomeView();

  void Refresh(PlacesGroup* foo =NULL);

protected:
  // Introspectable methods
  const gchar* GetName();
  const gchar* GetChildsName();
  void AddProperties(GVariantBuilder* builder);

private:
  static void DashVisible(GVariant* data, void* val);
  void OnShortcutClicked(PlacesTile* _tile);
  static void OnKeyChanged(GConfClient*    client,
                           guint           cnxn_id,
                           GConfEntry*     entry,
                           PlacesHomeView* self);
  void CreateShortcutFromExec(const char* exec,
                              const char* name,
                              std::vector<std::string>& alternatives);

private:
  nux::GridHLayout*        _layout;
  GConfClient* _client;
  std::vector<std::string> _browser_alternatives;
  std::vector<std::string> _photo_alternatives;
  std::vector<std::string> _email_alternatives;
  std::vector<std::string> _music_alternatives;

  struct timespec time_diff(struct timespec start, struct timespec end);
  struct timespec _last_activate_time;

  guint _ubus_handle;
  guint _browser_gconf_notify;
  guint _mail_gconf_notify;
  guint _media_gconf_notify;
};

}
#endif
