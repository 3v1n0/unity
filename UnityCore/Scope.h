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

#ifndef UNITY_SCOPE_H
#define UNITY_SCOPE_H

#include <boost/noncopyable.hpp>
#include <sigc++/trackable.h>

#include "ScopeProxyInterface.h"
#include "Preview.h"
#include "Result.h"

namespace unity
{
namespace dash
{

namespace
{
#define G_SCOPE_ERROR g_scope_error_quark ()
typedef enum
{
  G_SCOPE_ERROR_NO_ACTIVATION_HANDLER  = (1 << 0),
  G_SCOPE_ERROR_INVALID_PREVIEW        = (2 << 0)
} GScopeError;

GQuark
g_scope_error_quark (void)
{
  return g_quark_from_static_string ("g-scope-error-quark");
}
}

class Scope : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<Scope> Ptr;

  Scope(ScopeData::Ptr const& scope_data);
  virtual ~Scope();

  // Must call this function after construction.
  virtual void Init();

  void Connect();

  nux::ROProperty<std::string> id;
  nux::ROProperty<bool> connected;

  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> results_dirty;
  nux::ROProperty<bool> is_master;
  nux::ROProperty<std::string> search_hint;
  nux::RWProperty<ScopeViewType> view_type;
  nux::RWProperty<std::string> form_factor;

  nux::ROProperty<Results::Ptr> results;
  nux::ROProperty<Filters::Ptr> filters;
  nux::ROProperty<Categories::Ptr> categories;
  nux::ROProperty<std::vector<unsigned int>> category_order;

  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> description;
  nux::ROProperty<std::string> icon_hint;
  nux::ROProperty<std::string> category_icon_hint;
  nux::ROProperty<std::vector<std::string>> keywords;
  nux::ROProperty<std::string> type;
  nux::ROProperty<std::string> query_pattern;
  nux::ROProperty<std::string> shortcut;

  virtual void Search(std::string const& search_hint, SearchCallback const& callback = nullptr, GCancellable* cancellable = nullptr);
  virtual void Search(std::string const& search_hint, glib::HintsMap const&, SearchCallback const& callback = nullptr, GCancellable* cancellable = nullptr);

  virtual void Activate(LocalResult const& result, ActivateCallback const& callback = nullptr, GCancellable* cancellable = nullptr);

  typedef std::function<void(LocalResult const&, Preview::Ptr const&, glib::Error const&)> PreviewCallback;
  virtual void Preview(LocalResult const& result, PreviewCallback const& callback = nullptr, GCancellable* cancellable = nullptr);

  virtual void ActivatePreviewAction(Preview::ActionPtr const& action,
                                     LocalResult const& result,
                                     glib::HintsMap const& hints,
                                     ActivateCallback const& callback = nullptr,
                                     GCancellable* cancellable = nullptr);

  virtual Results::Ptr GetResultsForCategory(unsigned category) const;


  sigc::signal<void, LocalResult const&, ScopeHandledType, glib::HintsMap const&> activated;
  sigc::signal<void, LocalResult const&, Preview::Ptr const&> preview_ready;

protected:
  virtual ScopeProxyInterface::Ptr CreateProxyInterface() const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;

  friend class TestScope;
};

} // namespace dash
} // namespace unity

#endif // UNITY_SCOPE_H
