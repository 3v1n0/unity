/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITYSHELL_COMPIZ_SHORTCUT_MODELLER_H
#define UNITYSHELL_COMPIZ_SHORTCUT_MODELLER_H

#include "AbstractShortcutModeller.h"

namespace unity
{
namespace shortcut
{

class CompizModeller : public AbstractModeller
{
public:
  CompizModeller();
  Model::Ptr GetCurrentModel() const;

private:
  void BuildModel(int hsize, int vsize);

  void AddLauncherHints(std::list<shortcut::AbstractHint::Ptr> &hints);
  void AddDashHints(std::list<shortcut::AbstractHint::Ptr> &hints);
  void AddMenuHints(std::list<shortcut::AbstractHint::Ptr> &hints);
  void AddSwitcherHints(std::list<shortcut::AbstractHint::Ptr> &hints);
  void AddWorkspaceHints(std::list<shortcut::AbstractHint::Ptr> &hints);
  void AddWindowsHints(std::list<shortcut::AbstractHint::Ptr> &hints);

  Model::Ptr model_;
};

}
}

#endif
