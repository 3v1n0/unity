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

#include "IndicatorObjectFactoryRemote.h"

#include "IndicatorObjectProxyRemote.h"
#include "IndicatorObjectEntryProxyRemote.h"
#include "IndicatorObjectEntryProxy.h"
#include "Variant.h"

#include "Nux/Nux.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GLWindowManager.h"
#include <X11/Xlib.h>
#include <algorithm>

#define S_NAME  "com.canonical.Unity.Panel.Service"
#define S_PATH  "/com/canonical/Unity/Panel/Service"
#define S_IFACE "com.canonical.Unity.Panel.Service"

typedef struct
{
  GDBusProxy *proxy;
  gchar *entry_id;
  int x;
  int y;
  guint timestamp;
  guint32 button;

} ShowEntryData;


// Forwards
static void on_proxy_ready_cb (GObject      *source,
                               GAsyncResult *res,
                               gpointer      data);

static void on_proxy_signal_received (GDBusProxy *proxy,
                                      gchar      *sender_name,
                                      gchar      *signal_name,
                                      GVariant   *parameters,
                                      IndicatorObjectFactoryRemote *remote);

static void on_proxy_name_owner_changed (GDBusProxy *proxy,
                                         GParamSpec *pspec,
                                         IndicatorObjectFactoryRemote *remote);

static void on_sync_ready_cb (GObject      *source,
                              GAsyncResult *res,
                              gpointer      data);

static bool run_local_panel_service ();
static bool reconnect_to_service (gpointer data);

// Public Methods
IndicatorObjectFactoryRemote::IndicatorObjectFactoryRemote ()
: _proxy (NULL)
{
  Reconnect ();
}

IndicatorObjectFactoryRemote::~IndicatorObjectFactoryRemote ()
{
  if (G_IS_OBJECT (_proxy))
  {
    g_signal_handler_disconnect (_proxy, _proxy_signal_id);
    g_signal_handler_disconnect (_proxy, _proxy_name_id);
    g_object_unref (_proxy);
  }
  _proxy = NULL;

  {
    std::vector<IndicatorObjectProxy*>::iterator it;
    for (it = _indicators.begin(); it != _indicators.end(); it++)
    {
      IndicatorObjectProxyRemote *remote = static_cast<IndicatorObjectProxyRemote *> (*it);
      delete remote;
    }
    _indicators.erase (_indicators.begin (), _indicators.end ());
  }

  { // We cancel all our async callbacks from pending Sync() calls
    std::vector<SyncData *>::iterator it, eit = _sync_cancellables.end ();
    for (it = _sync_cancellables.begin (); it != eit; ++it)
    {
      SyncData *data = (*it);
      g_cancellable_cancel (data->_cancel);
      delete data;
    }
    _sync_cancellables.erase (_sync_cancellables.begin (), _sync_cancellables.end ());
  }
}

std::vector<IndicatorObjectProxy *>&
IndicatorObjectFactoryRemote::GetIndicatorObjects ()
{
  return _indicators;
}

void
IndicatorObjectFactoryRemote::ForceRefresh ()
{
}

void
IndicatorObjectFactoryRemote::Reconnect ()
{
  g_spawn_command_line_sync ("killall unity-panel-service", NULL, NULL, NULL, NULL);

  if (g_getenv ("PANEL_USE_LOCAL_SERVICE"))
  {
    run_local_panel_service ();
    g_timeout_add_seconds (1, (GSourceFunc)reconnect_to_service, this);
  }
  else
  {
    // We want to grab the Panel Service object. This is async, which is fine
    reconnect_to_service (this);
  }
}

void
IndicatorObjectFactoryRemote::OnRemoteProxyReady (GDBusProxy *proxy)
{
  if (_proxy)
  {
    // We've been connected before; We don't need new proxy, just continue
    // rocking with the old one.
    g_object_unref (proxy);
  }
  else
  {
    _proxy = proxy;

    // Connect to interesting signals
    _proxy_signal_id = g_signal_connect (_proxy, "g-signal",
                                         G_CALLBACK (on_proxy_signal_received), this);
    _proxy_name_id = g_signal_connect (_proxy, "notify::g-name-owner",
                                       G_CALLBACK (on_proxy_name_owner_changed), this);
  }

   SyncData * data = new SyncData (this);
  _sync_cancellables.push_back (data);
  g_dbus_proxy_call (_proxy,
                     "Sync",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     data->_cancel,
                     on_sync_ready_cb,
                     data);
}

static gboolean
send_show_entry (ShowEntryData *data)
{
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (G_IS_DBUS_PROXY (data->proxy), FALSE);
  
  /* Re-flush 'cos X is crap like that */
  Display* d = nux::GetGraphicsDisplay()->GetX11Display();
  XFlush (d);
  
  g_dbus_proxy_call (data->proxy,
                     "ShowEntry",
                     g_variant_new ("(suiii)",
                                    data->entry_id,
                                    0,
                                    data->x,
                                    data->y,
                                    data->button),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);

  g_free (data->entry_id);
  g_slice_free (ShowEntryData, data);
  return FALSE;
}

void
IndicatorObjectFactoryRemote::OnShowMenuRequestReceived (const char *entry_id,
                                                         int         x,
                                                         int         y,
                                                         guint       timestamp,
                                                         guint32     button)
{
  Display* d = nux::GetGraphicsDisplay()->GetX11Display();
  XUngrabPointer(d, CurrentTime);
  XFlush (d);

  // We have to do this because on certain systems X won't have time to 
  // respond to our request for XUngrabPointer and this will cause the
  // menu not to show
  ShowEntryData *data = g_slice_new0 (ShowEntryData);
  data->proxy = _proxy;
  data->entry_id = g_strdup (entry_id);
  data->x = x;
  data->y = y;
  data->timestamp = timestamp;
  data->button = button;

  g_timeout_add (0, (GSourceFunc)send_show_entry, data);

  // --------------------------------------------------------------------------
  // FIXME: This is a workaround until the non-paired events issue is fixed in
  // nux
  XButtonEvent ev = {
    ButtonRelease,
    0,
    False,
    d,
    0,
    0,
    0,
    CurrentTime,
    x, y,
    x, y,
    0,
    Button1,
    True
  };
  XEvent *e = (XEvent*)&ev;
  nux::GetGraphicsThread()->ProcessForeignEvent (e, NULL);
  // --------------------------------------------------------------------------
}

void
IndicatorObjectFactoryRemote::OnScrollReceived (const char *entry_id,
                                                       int delta)
{
  g_dbus_proxy_call (_proxy,
                     "ScrollEntry",
                     g_variant_new ("(si)",
                                    entry_id,
                                    delta),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
}

// We need to unset the last active entry and set the new one as active
void
IndicatorObjectFactoryRemote::OnEntryActivated (const char *entry_id)
{
  std::vector<IndicatorObjectProxy*>::iterator it;
  
  for (it = _indicators.begin(); it != _indicators.end(); ++it)
  {
    IndicatorObjectProxyRemote *object = static_cast<IndicatorObjectProxyRemote *> (*it);
    std::vector<IndicatorObjectEntryProxy*>::iterator it2;
  
    for (it2 = object->GetEntries ().begin(); it2 != object->GetEntries ().end(); ++it2)
    {
      IndicatorObjectEntryProxyRemote *entry = static_cast<IndicatorObjectEntryProxyRemote *> (*it2);

      entry->SetActive (g_strcmp0 (entry_id, entry->GetId ()) == 0);
    }
  }

  IndicatorObjectFactory::OnEntryActivated.emit (entry_id);
}

void
IndicatorObjectFactoryRemote::OnEntryActivateRequestReceived (const gchar *entry_id)
{
  OnEntryActivateRequest.emit (entry_id);
}

void
IndicatorObjectFactoryRemote::OnEntryShowNowChanged (const char *entry_id, bool show_now_state)
{
  std::vector<IndicatorObjectProxy*>::iterator it;
  
  for (it = _indicators.begin(); it != _indicators.end(); ++it)
  {
    IndicatorObjectProxyRemote *object = static_cast<IndicatorObjectProxyRemote *> (*it);
    std::vector<IndicatorObjectEntryProxy*>::iterator it2;
  
    for (it2 = object->GetEntries ().begin(); it2 != object->GetEntries ().end(); ++it2)
    {
      IndicatorObjectEntryProxyRemote *entry = static_cast<IndicatorObjectEntryProxyRemote *> (*it2);

      if (g_strcmp0 (entry_id, entry->GetId ()) == 0)
      {
        entry->OnShowNowChanged (show_now_state);
        return;
      }
    }
  }
}

IndicatorObjectProxyRemote *
IndicatorObjectFactoryRemote::IndicatorForID (const char *id)
{
  IndicatorObjectProxyRemote *remote = NULL;
  std::vector<IndicatorObjectProxy*>::iterator it;
  
  for (it = _indicators.begin(); it != _indicators.end(); it++)
  {
    IndicatorObjectProxyRemote *r = static_cast<IndicatorObjectProxyRemote *> (*it);

    if (g_strcmp0 (id, r->GetName ().c_str ()) == 0)
      {
        remote = r;
        break;
      }
  }

  if (remote == NULL)
    {
      // Create one
      remote = new IndicatorObjectProxyRemote (id);
      remote->OnShowMenuRequest.connect (sigc::mem_fun (this,
                                                        &IndicatorObjectFactoryRemote::OnShowMenuRequestReceived));
      remote->OnScroll.connect (sigc::mem_fun (this,
                                                &IndicatorObjectFactoryRemote::OnScrollReceived));

      _indicators.push_back (remote);

      OnObjectAdded.emit (remote);
    }

  return remote;
}

void
IndicatorObjectFactoryRemote::Sync (GVariant *args)
{    
  GVariantIter *iter            = NULL;
  gchar        *indicator_id    = NULL;
  gchar        *entry_id        = NULL;
  gchar        *label           = NULL;
  gboolean      label_sensitive = false;
  gboolean      label_visible   = false;
  guint32       image_type      = 0;
  gchar        *image_data      = NULL;
  gboolean      image_sensitive = false;
  gboolean      image_visible   = false;
  IndicatorObjectProxyRemote *current_proxy = NULL;
  gchar                      *current_proxy_id = NULL;

  // sanity check
  if (!args)
    return;

  g_variant_get (args, "(a(sssbbusbb))", &iter);
  while (g_variant_iter_loop (iter, "(sssbbusbb)",
                              &indicator_id,
                              &entry_id,
                              &label,
                              &label_sensitive,
                              &label_visible,
                              &image_type,
                              &image_data,
                              &image_sensitive,
                              &image_visible))
    {
      if (g_strcmp0 (current_proxy_id, indicator_id) != 0)
        {
          if (current_proxy)
            current_proxy->EndSync ();
          g_free (current_proxy_id);

          current_proxy_id = g_strdup (indicator_id);
          current_proxy = IndicatorForID (indicator_id);
          current_proxy->BeginSync ();
        }

      /* NULL entries (id == "") are just padding */
      if (g_strcmp0 (entry_id, "") != 0)
        current_proxy->AddEntry (entry_id,
                                 label,
                                 label_sensitive,
                                 label_visible,
                                 image_type,
                                 image_data,
                                 image_sensitive,
                                 image_visible);
    }
  if (current_proxy)
    current_proxy->EndSync ();
  
  g_free (current_proxy_id);
  g_variant_iter_free (iter);

  /* Notify listeners we have new data */
  OnSynced.emit ();
}

void
IndicatorObjectFactoryRemote::AddProperties (GVariantBuilder *builder)
{
  gchar *name = NULL;
  gchar *uname = NULL;
  
  g_object_get (_proxy,
                "g-name", &name,
                "g-name-owner", &uname,
                NULL);

  bool using_local_service = g_getenv ("PANEL_USE_LOCAL_SERVICE") != NULL;
  unity::variant::BuilderWrapper(builder)
    .add("backend", "remote")
    .add("service-name", name)
    .add("service-unique-name", uname)
    .add("using-local-service", using_local_service);

  g_free (name);
  g_free (uname);
}

GDBusProxy *
IndicatorObjectFactoryRemote::GetRemoteProxy ()
{
  return _proxy;
}
  
//
// C callbacks, they just link to class methods and aren't interesting
//

static bool
reconnect_to_service (gpointer data)
{
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                            NULL,
                            S_NAME,
                            S_PATH,
                            S_IFACE,
                            NULL,
                            on_proxy_ready_cb,
                            data);

  return false;
}

static void
on_proxy_ready_cb (GObject      *source,
                   GAsyncResult *res,
                   gpointer      data)
{
  IndicatorObjectFactoryRemote *remote = static_cast<IndicatorObjectFactoryRemote *> (data);
  GDBusProxy *proxy;
  GError     *error = NULL;
  static bool force_tried = false;
  char       *name_owner;

  proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (G_IS_DBUS_PROXY (proxy) && name_owner)
  {
    remote->OnRemoteProxyReady (G_DBUS_PROXY (proxy));
    g_free (name_owner);
    return;
  }
  else
  {
    if (force_tried)
    {
      printf ("\nWARNING: Unable to connect to the unity-panel-service %s\n",
              error ? error->message : "Unknown");
      if (error)
        g_error_free (error);
    }
    else
    {
      force_tried = true;
      run_local_panel_service ();

      g_timeout_add_seconds (2, (GSourceFunc)reconnect_to_service, remote);
    }
  }

  g_object_unref (proxy);
}

static bool
run_local_panel_service ()
{
  GError *error = NULL;

  // This is obviously hackish, but this part of the code is mostly hackish...
  // Let's attempt to run it from where we expect it to be
  char *cmd = g_strdup_printf ("%s/lib/unity/unity-panel-service", PREFIXDIR);
  printf ("\nWARNING: Couldn't load panel from installed services, so trying to"
          "load panel from known location: %s\n", cmd);

  g_spawn_command_line_async (cmd, &error);
  g_free (cmd);
  
  if (error)
  {
    printf ("\nWARNING: Unable to launch remote service manually: %s\n", error->message);
    g_error_free (error);
    return false;
  }
  return true;
}

static void
on_proxy_signal_received (GDBusProxy *proxy,
                          gchar      *sender_name,
                          gchar      *signal_name,
                          GVariant   *parameters,
                          IndicatorObjectFactoryRemote *remote)
{
  if (g_strcmp0 (signal_name, "EntryActivated") == 0)
  {
    remote->OnEntryActivated (g_variant_get_string (g_variant_get_child_value (parameters, 0), NULL));
  }
  else if (g_strcmp0 (signal_name, "EntryActivateRequest") == 0)
  {
    remote->OnEntryActivateRequestReceived (g_variant_get_string (g_variant_get_child_value (parameters, 0), NULL));
  }
  else if (g_strcmp0 (signal_name, "ReSync") == 0)
  {
    const gchar  *id = g_variant_get_string (g_variant_get_child_value (parameters, 0), NULL);
    bool          sync_one = !g_strcmp0 (id, "") == 0;

    SyncData *data = new SyncData (remote);
    remote->_sync_cancellables.push_back (data);
    
    g_dbus_proxy_call (proxy,
                       sync_one ? "SyncOne" : "Sync", 
                       sync_one ? g_variant_new ("(s)", id) : NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       data->_cancel,
                       on_sync_ready_cb,
                       data);
  }
  else if (g_strcmp0 (signal_name, "ActiveMenuPointerMotion") == 0)
  {
    int x=0, y=0;

    g_variant_get (parameters, "(ii)", &x, &y);

    remote->OnMenuPointerMoved.emit (x, y);
  }
  else if (g_strcmp0 (signal_name, "EntryShowNowChanged") == 0)
  {
    gchar    *id = NULL;
    gboolean  show_now_state;

    g_variant_get (parameters, "(sb)", &id, &show_now_state);

    remote->OnEntryShowNowChanged (id, show_now_state ? true : false);

    g_free (id);
  }
}

static void
on_proxy_name_owner_changed (GDBusProxy *proxy,
                             GParamSpec *pspec,
                             IndicatorObjectFactoryRemote *remote)
{
  char *name_owner;

  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (name_owner == NULL)
  {
    // The panel service has stopped for some reason.  Restart it if not in dev mode
    if (! g_getenv ("UNITY_DEV_MODE"))
      remote->Reconnect ();
  }

  g_free (name_owner);
}

static void
on_sync_ready_cb (GObject      *source,
                  GAsyncResult *res,
                  gpointer      data)
{
  SyncData     *sync_data = (SyncData *)data;
  IndicatorObjectFactoryRemote *remote = (IndicatorObjectFactoryRemote*)sync_data->_self;
  GVariant     *args;
  GError       *error = NULL;

  args = g_dbus_proxy_call_finish ((GDBusProxy*)source, res, &error);

  if (args == NULL)
    {
      g_warning ("Unable to perform Sync() on panel service: %s", error->message);
      g_error_free (error);
      return;
    }

  remote->Sync (args);

  std::vector<SyncData *>::iterator it = std::find (remote->_sync_cancellables.begin (),
                                                    remote->_sync_cancellables.end (),
                                                    sync_data);
  if (it != remote->_sync_cancellables.end ())
    remote->_sync_cancellables.erase (it);
  
  g_variant_unref (args);
  delete sync_data;
}
