// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef IM_TEXT_ENTRY_H
#define IM_TEXT_ENTRY_H

#include <Nux/Nux.h>
#include <Nux/TextEntry.h>

namespace unity
{

class IMTextEntry : public nux::TextEntry
{
  NUX_DECLARE_OBJECT_TYPE(IMTextEntry, nux::TextEntry);
public:
  IMTextEntry();
  virtual ~IMTextEntry() {}
  bool im_preedit();

protected:
  virtual void InsertText(std::string const& text);
  virtual void CopyClipboard();
  virtual void PasteClipboard();
  virtual void PastePrimaryClipboard();

  void Paste(bool primary = false);
};

}

#endif
