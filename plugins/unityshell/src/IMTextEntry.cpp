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

#include <gdk/gdkx.h>

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(IMTextEntry);

IMTextEntry::IMTextEntry()
  : TextEntry("", "", 80085)
  , im_context_(0)
  , im_context_simple_(0)
  , client_window_(0)
{
  im_context_ = gtk_im_multicontext_new();
//  gtk_im_context_set_use_preedit(im_context_, FALSE);
  sig_manager_.Add(new Signal<void, GtkIMContext*, char*>(im_context_, "commit", sigc::mem_fun(this, &IMTextEntry::OnIMCommit)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-changed", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditChanged)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-start", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditStart)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-end", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditEnd)));

  im_context_simple_ = gtk_im_context_simple_new();
  sig_manager_.Add(new Signal<void, GtkIMContext*, char*>(im_context_simple_, "commit", sigc::mem_fun(this, &IMTextEntry::OnIMCommit)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_simple_, "preedit-changed", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditChanged)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_simple_, "preedit-start", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditStart)));
  sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_simple_, "preedit-end", sigc::mem_fun(this, &IMTextEntry::OnIMPreeditEnd)));

  FocusChanged.connect(sigc::mem_fun(this, &IMTextEntry::OnFocusChanged));
  OnKeyNavFocusChange.connect(sigc::mem_fun(this, &IMTextEntry::OnFocusChanged));
}

IMTextEntry::~IMTextEntry()
{
  if (im_context_)
    g_object_unref(im_context_);
  if (im_context_simple_)
    g_object_unref(im_context_simple_);
  if (client_window_)
    g_object_unref(client_window_);
}

long IMTextEntry::ProcessEvent(nux::IEvent& ievent,
                               long traverse_info,
                               long pevent_info)
{
  if (!client_window_)
  {
    client_window_ = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), ievent.e_x11_window);
    gtk_im_context_set_client_window(im_context_, client_window_);
    gtk_im_context_set_client_window(im_context_simple_, client_window_);
  }
/*
  if (TryHandleEvent(ievent, traverse_info, pevent_info))
    return traverse_info | nux::eKeyEventSolved;
  else
    return TextEntry::ProcessEvent(ievent, traverse_info, pevent_info);
    */
  return traverse_info;
}

bool IMTextEntry::InspectKeyEvent(unsigned int eventType,
                                  unsigned int keysym,
                                  const char* character)
{
  if (TryHandleEvent(eventType, keysym, character))
    return true;
  else
    return TextEntry::InspectKeyEvent(eventType, keysym, character);
}

bool IMTextEntry::TryHandleEvent(unsigned int eventType,
                                 unsigned int keysym,
                                 const char* character)
{
  nux::Event event = nux::GetGraphicsThread()->GetWindow().GetCurrentEvent();
  if (!client_window_)
  {
    client_window_ = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), event.e_x11_window);
    gtk_im_context_set_client_window(im_context_, client_window_);
    gtk_im_context_set_client_window(im_context_simple_, client_window_);
    gtk_im_context_reset(im_context_);
    gtk_im_context_reset(im_context_simple_);
  }
  GdkEventKey ev;
  ev.type = eventType == nux::NUX_KEYDOWN ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
  ev.window = client_window_;
  ev.send_event = TRUE;
  ev.time = GDK_CURRENT_TIME;
  ev.state = 0;
  if (event.e_key_modifiers & NUX_STATE_CTRL)
    ev.state |= GDK_CONTROL_MASK;
  if (event.e_key_modifiers & NUX_STATE_SHIFT)
    ev.state |= GDK_SHIFT_MASK;
  ev.keyval = keysym;
  ev.length = 0;
  ev.string = NULL;
  ev.hardware_keycode = XKeysymToKeycode(GDK_WINDOW_XDISPLAY(client_window_),
                                         (KeySym)keysym);
  ev.group = 0;
  ev.is_modifier = 0;

  g_debug ("Filter: %d %u %u", gtk_im_context_filter_keypress(im_context_, &ev), ev.keyval, ev.hardware_keycode);
  gtk_im_context_filter_keypress(im_context_simple_, &ev);

  return false;
}

void IMTextEntry::OnFocusChanged(nux::Area* area)
{
  g_debug("Focus: %d", GetFocused());

  if (GetFocused())
  {
    gtk_im_context_focus_in(im_context_);
    gtk_im_context_focus_in(im_context_simple_);
  }
  else
  {
    gtk_im_context_focus_out(im_context_);
    gtk_im_context_focus_out(im_context_simple_);
  }
}

void IMTextEntry::OnIMCommit(GtkIMContext* context, char* str)
{
  g_debug("Commit: %s", str);
}

void IMTextEntry::OnIMPreeditChanged(GtkIMContext* context)
{
  g_debug("Preedit changed");
}

void IMTextEntry::OnIMPreeditStart(GtkIMContext* context)
{
  g_debug("Preedit start");
}

void IMTextEntry::OnIMPreeditEnd(GtkIMContext* context)
{
  g_debug("Preedit end");
}

}
}
