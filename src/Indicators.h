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

#ifndef INDICATOR_OBJECT_FACTORY_H
#define INDICATOR_OBJECT_FACTORY_H

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include "IndicatorObjectProxy.h"

class IndicatorObjectFactory : public sigc::trackable
{
public:

  // This could be empty, if the loading of Indicators is async, so you
  // should be listening to the signals too
  virtual std::vector<IndicatorObjectProxy *>& GetIndicatorObjects () = 0;

  // If the implementation supports it, you'll get a refreshed list of
  // Indicators (probably though a bunch of removed/added events)
  virtual void ForceRefresh () = 0;

  // For adding factory-specific properties
  virtual void AddProperties (GVariantBuilder *builder) = 0;

  // Signals
  sigc::signal<void, IndicatorObjectProxy *> OnObjectAdded;
  sigc::signal<void, IndicatorObjectProxy *> OnObjectRemoved;
  sigc::signal<void, int, int>               OnMenuPointerMoved;
  sigc::signal<void, const char *>           OnEntryActivateRequest;
  sigc::signal<void, const char *>           OnEntryActivated;
  sigc::signal<void>                         OnSynced;

protected:
  std::vector<IndicatorObjectProxy *>_indicators;
};

#endif // INDICATOR_OBJECT_FACTORY_H
