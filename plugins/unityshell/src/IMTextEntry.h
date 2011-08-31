// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef IM_TEXT_ENTRY_H
#define IM_TEXT_ENTRY_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <Nux/Nux.h>
#include <Nux/TextEntry.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace dash
{

using namespace unity::glib;
using namespace nux;

class IMTextEntry : public nux::TextEntry
{
  NUX_DECLARE_OBJECT_TYPE(IMTextEntry, nux::TextEntry);
public:
  IMTextEntry();
  ~IMTextEntry();

  nux::Property<std::string> preedit_string;

private:
  void CheckIMEnabled();
  void SetupSimpleIM();
  void SetupMultiIM();

  bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);
  bool TryHandleEvent(unsigned int eventType, unsigned int keysym, const char* character);
  void KeyEventToGdkEventKey(Event& event, GdkEventKey& gdk_event);
  inline void CheckValidClientWindow(Window window);
  bool TryHandleSpecial(unsigned int eventType, unsigned int keysym, const char* character);
  void Cut();
  void Copy();
  void Paste();

  void OnFocusChanged(nux::Area* area);
  void OnCommit(GtkIMContext* context, char* str);
  void OnPreeditChanged(GtkIMContext* context);
  void OnPreeditStart(GtkIMContext* context);
  void OnPreeditEnd(GtkIMContext* context);

  void OnMouseButtonUp(int x, int y, unsigned long bflags, unsigned long kflags);

 private:
  glib::SignalManager sig_manager_;
  GtkIMContext* im_context_;
  GdkWindow* client_window_;
  bool im_enabled_;
  bool im_active_;
};

}
}

#endif
