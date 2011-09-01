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

#include "config.h"

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesHomeView.h"

#include "PlacesSimpleTile.h"
#include "PlacesStyle.h"
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include <string>
#include <vector>

#define DELTA_DOUBLE_REQUEST 500000000

using namespace unity;

enum
{
  TYPE_PLACE = 0,
  TYPE_EXEC
};

class Shortcut : public PlacesSimpleTile
{
public:
  Shortcut(const char* icon, const char* name, int size)
    : PlacesSimpleTile(icon, name, size),
      _id(0),
      _place_id(NULL),
      _place_section(0),
      _exec(NULL)
  {
    SetDndEnabled(false, false);
  }

  ~Shortcut()
  {
    g_free(_place_id);
    g_free(_exec);
  }

  int      _id;
  gchar*   _place_id;
  guint32  _place_section;
  char*    _exec;
};

PlacesHomeView::PlacesHomeView()
  : _ubus_handle(0)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  SetName(_("Shortcuts"));
  SetIcon(PKGDATADIR"/shortcuts_group_icon.png");
  SetDrawSeparator(false);

  _layout = new nux::GridHLayout(NUX_TRACKER_LOCATION);
  _layout->SetReconfigureParentLayoutOnGeometryChange(true);
  SetChildLayout(_layout);

  _layout->ForceChildrenSize(true);
  _layout->SetChildrenSize(style->GetHomeTileWidth(), style->GetHomeTileHeight());
  _layout->EnablePartialVisibility(false);
  _layout->SetHeightMatchContent(true);
  _layout->SetHorizontalExternalMargin(32);
  _layout->SetVerticalInternalMargin(32);
  _layout->SetHorizontalInternalMargin(32);
  _layout->SetMinMaxSize((style->GetHomeTileWidth() * 4) + (32 * 5),
                         (style->GetHomeTileHeight() * 2) + 32);

  _ubus_handle = ubus_server_register_interest(ubus_server_get_default(),
                                               UBUS_PLACE_VIEW_SHOWN,
                                               (UBusCallback) &PlacesHomeView::DashVisible,
                                               this);

  //In case the GConf key is invalid (e.g. when an app was uninstalled), we
  //rely on a fallback "whitelist" mechanism instead of showing nothing at all
  _browser_alternatives.push_back("firefox");
  _browser_alternatives.push_back("chromium-browser");
  _browser_alternatives.push_back("epiphany-browser");
  _browser_alternatives.push_back("midori");

  _photo_alternatives.push_back("shotwell");
  _photo_alternatives.push_back("f-spot");
  _photo_alternatives.push_back("gthumb");
  _photo_alternatives.push_back("gwenview");
  _photo_alternatives.push_back("eog");

  _email_alternatives.push_back("evolution");
  _email_alternatives.push_back("thunderbird");
  _email_alternatives.push_back("claws-mail");
  _email_alternatives.push_back("kmail");

  _music_alternatives.push_back("banshee-1");
  _music_alternatives.push_back("rhythmbox");
  _music_alternatives.push_back("totem");
  _music_alternatives.push_back("vlc");

  expanded.connect(sigc::mem_fun(this, &PlacesHomeView::Refresh));

  Refresh();
}

PlacesHomeView::~PlacesHomeView()
{
  if (_ubus_handle != 0)
    ubus_server_unregister_interest(ubus_server_get_default(), _ubus_handle);
}

void
PlacesHomeView::DashVisible(GVariant* data, void* val)
{
  PlacesHomeView* self = (PlacesHomeView*)val;
  self->Refresh();
}

void
PlacesHomeView::Refresh(PlacesGroup*foo)
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  Shortcut*   shortcut = NULL;
  gchar*      markup = NULL;
  const char* temp = "<big>%s</big>";
  int         icon_size = style->GetHomeTileIconSize();

  _layout->Clear();

  // Media Apps
  markup = g_strdup_printf(temp, _("Media Apps"));
  shortcut = new Shortcut(PKGDATADIR"/find_media_apps.png",
                          markup,
                          icon_size);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup("applications.lens?filter_type=media");
  shortcut->_place_section = 9;
  _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));
  g_free(markup);

  // Internet Apps
  markup = g_strdup_printf(temp, _("Internet Apps"));
  shortcut = new Shortcut(PKGDATADIR"/find_internet_apps.png",
                          markup,
                          icon_size);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup("applications.lens?filter_type=internet");
  shortcut->_place_section = 8;
  _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));
  g_free(markup);

  // More Apps
  markup = g_strdup_printf(temp, _("More Apps"));
  shortcut = new Shortcut(PKGDATADIR"/find_more_apps.png",
                          markup,
                          icon_size);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup("applications.lens");
  shortcut->_place_section = 0;
  _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));
  g_free(markup);

  // Find Files
  markup = g_strdup_printf(temp, _("Find Files"));
  shortcut = new Shortcut(PKGDATADIR"/find_files.png",
                          markup,
                          icon_size);
  shortcut->_id = TYPE_PLACE;
  shortcut->_place_id = g_strdup("files.lens");
  shortcut->_place_section = 0;
  _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);
  shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));
  g_free(markup);

  // Browser
  CreateShortcutFromMime("x-scheme-handler/http", _("Browse the Web"), _browser_alternatives);

  // Photos
  // FIXME: Need to figure out the default
  CreateShortcutFromExec("shotwell", _("View Photos"), _photo_alternatives);

  CreateShortcutFromMime("x-scheme-handler/mailto", _("Check Email"), _email_alternatives);

  CreateShortcutFromMime("audio/x-vorbis+ogg", _("Listen to Music"), _music_alternatives);

  SetExpanded(true);
  SetCounts(8, 8);

  QueueDraw();
  _layout->QueueDraw();
  QueueRelayout();
}

void
PlacesHomeView::CreateShortcutFromExec(const char* exec,
                                       const char* name,
                                       std::vector<std::string>& alternatives)
{
  PlacesStyle*     style = PlacesStyle::GetDefault();
  Shortcut*        shortcut = NULL;
  gchar*           id;
  gchar*           markup;
  gchar*           icon;
  gchar*           real_exec;
  GDesktopAppInfo* info;

  markup = g_strdup_printf("<big>%s</big>", name);

  // We're going to try and create a desktop id from a exec string. Now, this is hairy at the
  // best of times but the following is the closest best-guess without having to do D-Bus
  // roundtrips to BAMF.
  if (exec)
  {
    char* basename;

    if (exec[0] == '/')
      basename = g_path_get_basename(exec);
    else
      basename = g_strdup(exec);

    id = g_strdup_printf("%s.desktop", basename);

    g_free(basename);
  }
  else
  {
    id = g_strdup_printf("%s.desktop", alternatives[0].c_str());
  }

  info = g_desktop_app_info_new(id);
  std::vector<std::string>::iterator iter = alternatives.begin();
  while (iter != alternatives.end())
  {
    if (!G_IS_DESKTOP_APP_INFO(info))
    {
      id = g_strdup_printf("%s.desktop", (*iter).c_str());
      info = g_desktop_app_info_new(id);
      iter++;
    }

    if (G_IS_DESKTOP_APP_INFO(info))
    {
      icon = g_icon_to_string(g_app_info_get_icon(G_APP_INFO(info)));
      real_exec = g_strdup(g_app_info_get_executable(G_APP_INFO(info)));

      shortcut = new Shortcut(icon, markup, style->GetHomeTileIconSize());
      shortcut->_id = TYPE_EXEC;
      shortcut->_exec = real_exec;
      _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);
      shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));

      g_free(icon);

      break;
    }
  }

  g_free(id);
  g_free(markup);
}

void PlacesHomeView::CreateShortcutFromMime(const char* mime,
                                            const char* name,
                                            std::vector<std::string>& alternatives)
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  GAppInfo* info = g_app_info_get_default_for_type(mime, FALSE);

  // If it was invalid check alternatives for backup
  if (!G_IS_DESKTOP_APP_INFO(info))
  {
    for (auto alt: alternatives)
    {
      std::string id = alt + ".desktop";
      info = G_APP_INFO(g_desktop_app_info_new(id.c_str()));
      
      if (G_IS_DESKTOP_APP_INFO(info))
        break;
    }
  }

  if (G_IS_DESKTOP_APP_INFO(info))
  {
    glib::String icon(g_icon_to_string(g_app_info_get_icon(G_APP_INFO(info))));
    glib::String markup(g_strdup_printf("<big>%s</big>", name));
    
    Shortcut*   shortcut = new Shortcut(icon.Value(), markup.Value(), style->GetHomeTileIconSize());
    shortcut->_id = TYPE_EXEC;
    shortcut->_exec = g_strdup (g_app_info_get_executable(G_APP_INFO(info)));;
    shortcut->sigClick.connect(sigc::mem_fun(this, &PlacesHomeView::OnShortcutClicked));
    _layout->AddView(shortcut, 1, nux::eLeft, nux::eFull);

    g_object_unref(info);
  }
}

void
PlacesHomeView::OnShortcutClicked(PlacesTile* tile)
{
  Shortcut* shortcut = static_cast<Shortcut*>(tile);
  int id = shortcut->_id;

  if (id == TYPE_PLACE)
  {
    ubus_server_send_message(ubus_server_get_default(),
                             UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                             g_variant_new("(sus)",
                                           shortcut->_place_id,
                                           shortcut->_place_section,
                                           ""));
  }
  else if (id == TYPE_EXEC)
  {
    GError* error = NULL;

    if (!g_spawn_command_line_async(shortcut->_exec, &error))
    {
      g_warning("%s: Unable to launch %s: %s",
                G_STRFUNC,
                shortcut->_exec,
                error->message);
      g_error_free(error);
    }

    ubus_server_send_message(ubus_server_get_default(),
                             UBUS_PLACE_VIEW_CLOSE_REQUEST,
                             NULL);
  }
}

const gchar* PlacesHomeView::GetName()
{
  return "PlacesHomeView";
}

const gchar*
PlacesHomeView::GetChildsName()
{
  return "";
}

void PlacesHomeView::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}

