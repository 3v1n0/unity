// -*- Mode: C++; indent-tabs-mode: ni; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "IMTextEntry.h"
#include <gtk/gtk.h>

namespace unity
{
NUX_IMPLEMENT_OBJECT_TYPE(IMTextEntry);

IMTextEntry::IMTextEntry()
: TextEntry("", NUX_TRACKER_LOCATION)
{}

void IMTextEntry::CopyClipboard()
{
  int start, end;

  if (GetSelectionBounds(&start, &end))
  {
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, text_.c_str() + start, end - start);
  }
}

void IMTextEntry::PasteClipboard()
{
  Paste();
}

void IMTextEntry::PastePrimaryClipboard()
{
  Paste(true);
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
    QueueRefresh(true, true);
  }
}

bool IMTextEntry::im_preedit()
{
  return !preedit_.empty();
}
}
