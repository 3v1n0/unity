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

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{

namespace
{
nux::logging::Logger logger("unity.imtextentry");
}

NUX_IMPLEMENT_OBJECT_TYPE(IMTextEntry);

IMTextEntry::IMTextEntry()
: TextEntry("", NUX_TRACKER_LOCATION)
, im_context_(0)
, client_window_(0)
, focused_(false)
{
  SetupSimpleIM();

  mouse_up.connect(sigc::mem_fun(this, &IMTextEntry::OnMouseButtonUp));
}

bool IMTextEntry::InspectKeyEvent(unsigned int event_type,
                                  unsigned int keysym,
                                  const char* character)
{

  bool propagate_event = !(TryHandleEvent(event_type, keysym, character));

  if (propagate_event)
  {
    propagate_event = TryHandleSpecial(event_type, keysym, character);
  }

  if (propagate_event)
  {
    propagate_event = TextEntry::InspectKeyEvent(event_type, keysym, character);
  }

  return propagate_event;
}

bool IMTextEntry::TryHandleSpecial(unsigned int eventType, unsigned int keysym, const char* character)
{
  nux::Event event = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent();
  unsigned int keyval = keysym;
  bool shift = (event.GetKeyState() & NUX_STATE_SHIFT);
  bool ctrl = (event.GetKeyState() & NUX_STATE_CTRL);

  /* If there is preedit, handle the event else where, but we
     want to be able to copy/paste while ibus is active */
  if (!preedit_.empty())
    return true;

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
  else if (keyval == NUX_VK_TAB || keyval == NUX_VK_LEFT_TAB)
  {
    return true;
  }
  else
  {
    return true;
  }
  return false;
}

void IMTextEntry::Cut()
{
  Copy();
  DeleteSelection();
  QueueRefresh (true, true);
}

void IMTextEntry::Copy()
{
  int start=0, end=0;
  if (GetSelectionBounds(&start, &end))
  {
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, text_.c_str() + start, end - start);
  }
}

void IMTextEntry::Paste(bool primary)
{
  GdkAtom origin = primary ? GDK_SELECTION_PRIMARY : GDK_SELECTION_CLIPBOARD;
  GtkClipboard* clip = gtk_clipboard_get(origin);

  auto callback = [](GtkClipboard* clip, const char* text, gpointer user_data)
   {
     IMTextEntry* self = static_cast<IMTextEntry*>(user_data);
     if (text)
      self->InsertText(std::string(text));
   };

  gtk_clipboard_request_text(clip, callback, this);
}

void IMTextEntry::InsertText(std::string const& text)
{
  DeleteSelection();

  if (!text.empty())
  {
    std::string new_text(GetText());
    new_text.insert(cursor_, text);

    int cursor = cursor_;
    SetText(new_text.c_str());
    SetCursor(cursor + text.length());
    QueueRefresh (true, true);
    text_changed.emit(this);
  }
}

void IMTextEntry::OnMouseButtonUp(int x, int y, unsigned long bflags, unsigned long kflags)
{
  int button = nux::GetEventButton(bflags);

  if (button == 2)
  { 
    SetCursor(XYToTextIndex(x,y));
    Paste(true);
  } 
}

void IMTextEntry::KeyEventToGdkEventKey(Event& event, GdkEventKey& gdk_event)
{
  gdk_event.type = event.type == nux::NUX_KEYDOWN ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
  gdk_event.window = client_window_;
  gdk_event.send_event = FALSE;
  gdk_event.time = event.x11_timestamp;
  gdk_event.state = event.x11_key_state;
  gdk_event.keyval = event.x11_keysym;

  gchar* txt = const_cast<gchar*>(event.GetText());
  gdk_event.length = strlen(txt);
  gdk_event.string = txt;

  gdk_event.hardware_keycode = event.x11_keycode;
  gdk_event.group = 0;
  gdk_event.is_modifier = 0;
}

inline void IMTextEntry::CheckValidClientWindow(Window window)
{
  if (!client_window_)
  {
    client_window_ = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), window);
    gtk_im_context_set_client_window(im_context_, client_window_);

    if (1/*focused_*/)
    {
      gtk_im_context_focus_in(im_context_);
    }
  }
}

bool IMTextEntry::TryHandleEvent(unsigned int eventType,
                                 unsigned int keysym,
                                 const char* character)
{
  nux::Event event = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent();
  
  CheckValidClientWindow(event.x11_window);
  
  GdkEventKey ev;
  KeyEventToGdkEventKey(event, ev);

  return gtk_im_context_filter_keypress(im_context_, &ev);
}

void IMTextEntry::OnCommit(GtkIMContext* context, char* str)
{
  LOG_DEBUG(logger) << "Commit: " << str;
  DeleteSelection();

  if (str)
  {
    std::string new_text = GetText();
    new_text.insert(cursor_, str);
    
    int cursor = cursor_;
    SetText(new_text.c_str());
    SetCursor(cursor + strlen(str));
    QueueRefresh (true, true);
    text_changed.emit(this);
  }
}

void IMTextEntry::SetupSimpleIM()
{
  im_context_ = gtk_im_context_simple_new();
  
  sig_manager_.Add(new Signal<void, GtkIMContext*, char*>(im_context_, "commit", sigc::mem_fun(this, &IMTextEntry::OnCommit)));
  // sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-changed", sigc::mem_fun(this, &IMTextEntry::OnPreeditChanged)));
  // sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-start", sigc::mem_fun(this, &IMTextEntry::OnPreeditStart)));
  // sig_manager_.Add(new Signal<void, GtkIMContext*>(im_context_, "preedit-end", sigc::mem_fun(this, &IMTextEntry::OnPreeditEnd)));
}

}
