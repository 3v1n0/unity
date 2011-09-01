// -*- Mode: C++; indent-tabs-mode: ni; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Pubic License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it wi be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more detais.
 *
 * You shoud have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "config.h"

#include "IMTextEntry.h"

#include <boost/lexical_cast.hpp>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.imtextentry");
}

NUX_IMPLEMENT_OBJECT_TYPE(IMTextEntry);

IMTextEntry::IMTextEntry()
  : TextEntry("", "", 80085)
  , preedit_string("")
  , im_context_(0)
  , client_window_(0)
  , im_enabled_(false)
  , im_active_(false)
  , focused_(false)
{
  g_setenv("IBUS_ENABLE_SYNC_MODE", "1", TRUE);
  CheckIMEnabled();
  im_enabled_ ? SetupMultiIM() : SetupSimpleIM();

  FocusChanged.connect([&] (nux::Area*) { GetFocused() ? OnFocusIn() : OnFocusOut(); });
  mouse_up.connect(sigc::mem_fun(this, &IMTextEntry::OnMouseButtonUp));
}

IMTextEntry::~IMTextEntry()
{
  if (im_context_)
    g_object_unref(im_context_);
  if (client_window_)
    g_object_unref(client_window_);
}

void IMTextEntry::CheckIMEnabled()
{
  const char* module = g_getenv("GTK_IM_MODULE");
  if (module &&
      g_strcmp0(module, "") &&
      g_strcmp0(module, "gtk-im-context-simple"))
    im_enabled_ = true;

  LOG_DEBUG(logger) << "Input method support is "
                    << (im_enabled_ ? "enabled" : "disabled");
}

void IMTextEntry::SetupSimpleIM()
{
  im_context_ = gtk_im_context_simple_new();
  
  sig_manager_.Add(new Signal<void, GtkIMContext*, char*>(im_context_, "commit", sigc::mem_fun(this, &IMTextEntry::OnCommit)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-changed", sigc::mem_fun(this, &IMTextEntry::OnPreeditChanged)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-start", sigc::mem_fun(this, &IMTextEntry::OnPreeditStart)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-end", sigc::mem_fun(this, &IMTextEntry::OnPreeditEnd)));
}

void IMTextEntry::SetupMultiIM()
{
  im_context_ = gtk_im_multicontext_new();
  
  sig_manager_.Add(new Signal<void, GtkIMContext*, char*>(im_context_, "commit", sigc::mem_fun(this, &IMTextEntry::OnCommit)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-changed", sigc::mem_fun(this, &IMTextEntry::OnPreeditChanged)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-start", sigc::mem_fun(this, &IMTextEntry::OnPreeditStart)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-end", sigc::mem_fun(this, &IMTextEntry::OnPreeditEnd)));
}

bool IMTextEntry::InspectKeyEvent(unsigned int event_type,
                                  unsigned int keysym,
                                  const char* character)
{
  bool propagate_event = !(TryHandleEvent(event_type, keysym, character));

  LOG_DEBUG(logger) << "Input method "
                    << (im_enabled_ ? gtk_im_multicontext_get_context_id(GTK_IM_MULTICONTEXT(im_context_)) : "simple")
                    << " "
                    << (propagate_event ? "did not handle " : "handled ") 
                    << "event ("
                    << (event_type == NUX_KEYDOWN ? "press" : "release")
                    << ") ";

  if (propagate_event)
    propagate_event = !TryHandleSpecial(event_type, keysym, character);

  if (propagate_event)
  {
    text_input_mode_ = event_type == NUX_KEYDOWN;
    propagate_event = TextEntry::InspectKeyEvent(event_type, keysym, character);
    text_input_mode_ = false;

    UpdateCursorLocation();
  }
  return propagate_event;
}

bool IMTextEntry::TryHandleEvent(unsigned int eventType,
                                 unsigned int keysym,
                                 const char* character)
{
  nux::Event event = nux::GetGraphicsThread()->GetWindow().GetCurrentEvent();
  
  CheckValidClientWindow(event.e_x11_window);
  
  GdkEventKey ev;
  KeyEventToGdkEventKey(event, ev);

  return gtk_im_context_filter_keypress(im_context_, &ev);
}

inline void IMTextEntry::CheckValidClientWindow(Window window)
{
  if (!client_window_)
  {
    client_window_ = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), window);
    gtk_im_context_set_client_window(im_context_, client_window_);

    if (focused_)
    {
      gtk_im_context_focus_in(im_context_);
    }
  }
}

void IMTextEntry::KeyEventToGdkEventKey(Event& event, GdkEventKey& gdk_event)
{
  gdk_event.type = event.e_event == nux::NUX_KEYDOWN ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
  gdk_event.window = client_window_;
  gdk_event.send_event = FALSE;
  gdk_event.time = event.e_x11_timestamp;
  gdk_event.state = event.e_x11_state;
  gdk_event.keyval = event.e_keysym;

  gchar* txt = const_cast<gchar*>(event.GetText());
  gdk_event.length = strlen(txt);
  gdk_event.string = txt;

  gdk_event.hardware_keycode = event.e_x11_keycode;
  gdk_event.group = 0;
  gdk_event.is_modifier = 0;
}

bool IMTextEntry::TryHandleSpecial(unsigned int eventType, unsigned int keysym, const char* character)
{
  nux::Event event = nux::GetGraphicsThread()->GetWindow().GetCurrentEvent();
  unsigned int keyval = keysym;
  bool shift = (event.GetKeyState() & NUX_STATE_SHIFT);
  bool ctrl = (event.GetKeyState() & NUX_STATE_CTRL);

  if (eventType != NUX_KEYDOWN)
    return false;

  if (((keyval == NUX_VK_x) && ctrl && !shift) ||
      ((keyval == NUX_VK_DELETE) && shift && !ctrl))
  {
    Cut();
  }
  else if (((keyval == NUX_VK_c) && ctrl && (!shift)) ||
           ((keyval == NUX_VK_INSERT) && ctrl && (!shift)))
  {
    Copy();
  }
  else if (((keyval == NUX_VK_v) && ctrl && (!shift)) ||
      ((keyval == NUX_VK_INSERT) && shift && (!ctrl)))
  {
    Paste();
  }
  else
  {
    return false;
  }
  return true;
}

void IMTextEntry::Cut()
{
  Copy();
  DeleteSelection();
}

void IMTextEntry::Copy()
{
  int start=0, end=0;
  if (GetSelectionBounds(&start, &end))
  {
    GtkClipboard* clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                       GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, _text.c_str() + start, end - start);
  }
}

void IMTextEntry::Paste()
{
  GtkClipboard* clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
                                                     GDK_SELECTION_CLIPBOARD);
  auto callback = [](GtkClipboard* clip, const char* text, gpointer user_data)
   {
     IMTextEntry* self = static_cast<IMTextEntry*>(user_data);
     self->OnCommit (self->im_context_, const_cast<char*>(text));
   };

  gtk_clipboard_request_text(clip, callback, this);
}

void IMTextEntry::OnCommit(GtkIMContext* context, char* str)
{
  LOG_DEBUG(logger) << "Commit: " << str;
  DeleteSelection();
  if (str)
  {
    std::string new_text = GetText() + str;
    int cursor = cursor_;
    SetText(new_text.c_str());
    SetCursor(cursor + strlen(str));
    UpdateCursorLocation();
  }
}

void IMTextEntry::OnPreeditChanged(GtkIMContext* context)
{
  glib::String preedit;
  int cursor_pos = -1;

  gtk_im_context_get_preedit_string(context, &preedit, NULL, &cursor_pos);

  LOG_DEBUG(logger) << "Preedit changed: " << preedit;

  preedit_string = preedit.Str();
}

void IMTextEntry::OnPreeditStart(GtkIMContext* context)
{
  preedit_string = "";
  im_active_ = true;

  LOG_DEBUG(logger) << "Preedit start";
}

void IMTextEntry::OnPreeditEnd(GtkIMContext* context)
{
  preedit_string = "";
  im_active_ = false;
  gtk_im_context_reset(im_context_);

  LOG_DEBUG(logger) << "Preedit ended";
}

void IMTextEntry::OnFocusIn()
{
  focused_ = true;
  gtk_im_context_focus_in(im_context_);
  gtk_im_context_reset(im_context_);
  UpdateCursorLocation();
}

void IMTextEntry::OnFocusOut()
{
  focused_ = false;
  gtk_im_context_focus_out(im_context_);
}

void IMTextEntry::UpdateCursorLocation()
{
  nux::Rect strong, weak;
  GetCursorRects(&strong, &weak);
  nux::Geometry geo = GetGeometry();
  
  GdkRectangle area = { strong.x + geo.x, strong.y + geo.y, strong.width, strong.height };
  gtk_im_context_set_cursor_location(im_context_, &area);
}

void IMTextEntry::OnMouseButtonUp(int x, int y, unsigned long bflags, unsigned long kflags)
{
  if (nux::GetEventButton(bflags) == 3 && im_enabled_)
  {
    GtkWidget* menu = gtk_menu_new();
    gtk_im_multicontext_append_menuitems(GTK_IM_MULTICONTEXT(im_context_),
                                          GTK_MENU_SHELL(menu));
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, GDK_CURRENT_TIME);
  }
}

}
}
