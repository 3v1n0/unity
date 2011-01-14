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

#ifndef INDICATOR_OBJECT_PROXY_H
#define INDICATOR_OBJECT_PROXY_H

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/sigc++.h>

#include "IndicatorObjectEntryProxy.h"

class IndicatorObjectProxy : public sigc::trackable
{
public:

  // The name of the indicator that this proxy represents
  virtual std::string& GetName () = 0;

  // The current list of IndicatorObjectEntryProxys
  virtual std::vector<IndicatorObjectEntryProxy *>& GetEntries () = 0;

  // Signals
  sigc::signal<void, IndicatorObjectEntryProxy *> OnEntryAdded;
  sigc::signal<void, IndicatorObjectEntryProxy *> OnEntryRemoved;
  sigc::signal<void, IndicatorObjectEntryProxy *> OnEntryMoved;

protected:
  std::vector<IndicatorObjectEntryProxy *> _entries;
};

#endif // INDICATOR_OBJECT_PROXY_H
