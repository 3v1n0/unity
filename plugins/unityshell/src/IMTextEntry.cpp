// -*- Mode: C++; indent-tabs-mode: ni; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonica Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Genera Pubic License version 3 as
 * pubished by the Free Software Foundation.
 *
 * This program is distributed in the hope that it wi be usefu,
 * but WITHOUT ANY WARRANTY; without even the impied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Genera Pubic License for more detais.
 *
 * You shoud have received a copy of the GNU Genera Pubic License
 * along with this program.  If not, see <http://www.gnu.org/icenses/>.
 *
 * Authored by: Nei Jagdish Pate <nei.pate@canonica.com>
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
{
  CheckIMEnabled();
  //FIXME: Make event forwarding work before enabling
  // im_enabled_ ? SetupMultiIM() : SetupSimpleIM();
  SetupSimpleIM();

  FocusChanged.connect(sigc::mem_fun(this, &IMTextEntry::OnFocusChanged));
  OnKeyNavFocusChange.connect(sigc::mem_fun(this, &IMTextEntry::OnFocusChanged));
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

    LOG_DEBUG(logger) << "Input method ("
                    << (im_enabled_ ? gtk_im_multicontext_get_context_id(GTK_IM_MULTICONTEXT(im_context_)) : "simple")
                    << ") "
                    << (propagate_event ? "did not handle " : "handled ") 
                    << "event ("
                    << (event_type == NUX_KEYDOWN ? "press" : "release")
                    << ") ";

  std::string preedit = preedit_string;
  if (preedit.length() < 1 && 
      (keysym == NUX_VK_ENTER ||
       keysym == NUX_KP_ENTER ||
       keysym == NUX_VK_UP ||
       keysym == NUX_VK_DOWN ||
       keysym == NUX_VK_LEFT ||
       keysym == NUX_VK_RIGHT ||
       keysym == NUX_VK_LEFT_TAB ||
       keysym == NUX_VK_TAB ||
       keysym == NUX_VK_ESCAPE ||
       keysym == NUX_VK_DELETE ||
       keysym == NUX_VK_BACKSPACE ||
       keysym == NUX_VK_HOME ||
       keysym == NUX_VK_END))
  {
    propagate_event = true;
  }

  return propagate_event ? TextEntry::InspectKeyEvent(event_type, keysym, character)
                         : propagate_event;
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

    if (GetFocused())
    {
      gtk_im_context_focus_in(im_context_);
    }
  }
}

void IMTextEntry::KeyEventToGdkEventKey(Event& event, GdkEventKey& gdk_event)
{
  gdk_event.type = event.e_event == nux::NUX_KEYDOWN ? GDK_KEY_PRESS : GDK_KEY_RELEASE;

  gdk_event.window = client_window_;
  gdk_event.window = 0;
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

void IMTextEntry::OnFocusChanged(nux::Area* area)
{

  LOG_DEBUG(logger) << "Focus changed " << boost::lexical_cast<bool>(GetFocused());

  if (GetFocused())
  {
    gtk_im_context_focus_in(im_context_);
  }
  else
  {
    gtk_im_context_focus_out(im_context_);
    gtk_im_context_reset(im_context_);
  }
}

void IMTextEntry::OnCommit(GtkIMContext* context, char* str)
{
  LOG_DEBUG(logger) << "Commit: " << str;
  if (str)
  {
    std::string new_text = GetText() + str;
    int cursor = cursor_;
    SetText(new_text.c_str());
    SetCursor(cursor + strlen(str));
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
