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

bool IMTextEntry::InspectKeyEvent(unsigned int eventType,
                                  unsigned int keysym,
                                  const char* character)
{
  if (keysym == NUX_VK_ENTER ||
      keysym == NUX_KP_ENTER ||
      keysym == NUX_VK_UP ||
      keysym == NUX_VK_DOWN ||
      keysym == NUX_VK_LEFT ||
      keysym == NUX_VK_RIGHT ||
      keysym == NUX_VK_LEFT_TAB ||
      keysym == NUX_VK_TAB ||
      keysym == NUX_VK_ESCAPE ||
      keysym == NUX_VK_DELETE ||
      keysym == NUX_VK_BACKSPACE)
    {
      return true;
    }


  if (TryHandleEvent(eventType, keysym, character))
  {
    return false;
  }
  else
  {
    return TextEntry::InspectKeyEvent(eventType, keysym, character);
  }
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
  KeyEventToGdkEventKey(event, ev);

  return gtk_im_context_filter_keypress(im_context_simple_, &ev) ||
         gtk_im_context_filter_keypress(im_context_, &ev);
}

void IMTextEntry::KeyEventToGdkEventKey(Event& event, GdkEventKey& gdk_event)
{
  gdk_event.type = event.e_event == nux::NUX_KEYDOWN ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
  gdk_event.window = client_window_;
  gdk_event.send_event = TRUE;
  gdk_event.time = GDK_CURRENT_TIME;
  
  gdk_event.state = 0;
  if (event.e_key_modifiers & NUX_STATE_CTRL)
    gdk_event.state |= GDK_CONTROL_MASK;
  if (event.e_key_modifiers & NUX_STATE_SHIFT)
    gdk_event.state |= GDK_SHIFT_MASK;

  gdk_event.keyval = event.e_keysym;
  gdk_event.length = 0;
  gdk_event.string = NULL;
  gdk_event.hardware_keycode = event.e_x11_keycode;
  gdk_event.group = 0;
  gdk_event.is_modifier = 0;
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
  if (str)
  {
    std::string new_text = GetText() + str;
    g_debug("Commit: %s", str);
    int cursor = cursor_;
    SetText(new_text.c_str());
    SetCursor(cursor + strlen(str));
  }
}

void IMTextEntry::OnIMPreeditChanged(GtkIMContext* context)
{
  char* preedit_string = NULL;
  int cursor_pos = -1;

  gtk_im_context_get_preedit_string(context, &preedit_string, NULL, &cursor_pos);
  g_debug("Preedit changed: %s %d", preedit_string, cursor_pos);

  g_free(preedit_string);
}

void IMTextEntry::OnIMPreeditStart(GtkIMContext* context)
{
  char* preedit_string = NULL;
  int cursor_pos = -1;

  gtk_im_context_get_preedit_string(context, &preedit_string, NULL, &cursor_pos);
  g_debug("Preedit start: %s %d", preedit_string, cursor_pos);

  g_free(preedit_string);}

void IMTextEntry::OnIMPreeditEnd(GtkIMContext* context)
{
  char* preedit_string = NULL;
  int cursor_pos = -1;

  gtk_im_context_get_preedit_string(context, &preedit_string, NULL, &cursor_pos);
  g_debug("Preedit end: %s %d", preedit_string, cursor_pos);

  g_free(preedit_string);
}

}
}
