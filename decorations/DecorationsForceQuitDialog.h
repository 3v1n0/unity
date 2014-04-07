// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef __UNITY_DECORATIONS_FORCE_QUIT_DIALOG__
#define __UNITY_DECORATIONS_FORCE_QUIT_DIALOG__

#include <NuxCore/Property.h>
#include "CompizUtils.h"

namespace unity
{
namespace decoration
{

class ForceQuitDialog
{
public:
  typedef std::shared_ptr<ForceQuitDialog> Ptr;

  ForceQuitDialog(CompWindow*, Time);
  ~ForceQuitDialog();

  nux::Property<Time> time;

  void UpdateDialogPosition();

  sigc::signal<void> close_request;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // menu namespace
} // decoration namespace

#endif // __UNITY_DECORATIONS_FORCE_QUIT_DIALOG__
