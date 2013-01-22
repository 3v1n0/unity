/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef UNITY_GSETTINGS_SCOPES_H
#define UNITY_GSETTINGS_SCOPES_H

#include "Scopes.h"

namespace unity
{
namespace dash
{

class GSettingsScopesReader : public ScopesReader
{
public:
  typedef std::shared_ptr<GSettingsScopesReader> Ptr;
  GSettingsScopesReader();

  virtual void LoadScopes();
  virtual ScopeDataList const& GetScopesData() const;
  virtual ScopeData::Ptr GetScopeDataById(std::string const& scope_id) const;

  static GSettingsScopesReader::Ptr GetDefault();

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};


class GSettingsScopes : public Scopes
{
public:
  GSettingsScopes();
};

} // namespace dash
} // namespace unity

#endif // UNITY_GSETTINGS_SCOPES_H