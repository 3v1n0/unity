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

#include "Result.h"
#include <sigc++/bind.h>
#include "GLibWrapper.h"

namespace unity
{
namespace dash
{

namespace
{
enum ResultColumn
{
  URI,
  ICON_HINT,
  CATEGORY,
  RESULT_TYPE,
  MIMETYPE,
  TITLE,
  COMMENT,
  DND_URI,
  METADATA
};
}

Result::Result(DeeModel* model,
               DeeModelIter* iter,
               DeeModelTag* renderer_tag)
  : RowAdaptorBase(model, iter, renderer_tag)
{
  SetupGetters();
}

Result::Result(Result const& other)
  : RowAdaptorBase(other)
{
  SetupGetters();
}

Result& Result::operator=(Result const& other)
{
  RowAdaptorBase::operator=(other);
  SetupGetters();
  return *this;
}

void Result::SetupGetters()
{
  uri.SetGetterFunction(sigc::mem_fun(this, &Result::GetURI));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &Result::GetIconHint));
  category_index.SetGetterFunction(sigc::mem_fun(this, &Result::GetCategoryIndex));
  result_type.SetGetterFunction(sigc::mem_fun(this, &Result::GetResultType));
  mimetype.SetGetterFunction(sigc::mem_fun(this, &Result::GetMimeType));
  name.SetGetterFunction(sigc::mem_fun(this, &Result::GetName));
  comment.SetGetterFunction(sigc::mem_fun(this, &Result::GetComment));
  dnd_uri.SetGetterFunction(sigc::mem_fun(this, &Result::GetDndURI));
  hints.SetGetterFunction(sigc::mem_fun(this, &Result::GetHints));
}

std::string Result::GetURI() const { return GetStringAt(ResultColumn::URI); }
std::string Result::GetIconHint() const { return GetStringAt(ResultColumn::ICON_HINT); }
unsigned Result::GetCategoryIndex() const { return GetUIntAt(ResultColumn::CATEGORY); }
unsigned Result::GetResultType() const { return GetUIntAt(ResultColumn::RESULT_TYPE); }
std::string Result::GetMimeType() const { return GetStringAt(ResultColumn::MIMETYPE); }
std::string Result::GetName() const { return GetStringAt(ResultColumn::TITLE); }
std::string Result::GetComment() const { return GetStringAt(ResultColumn::COMMENT); }
std::string Result::GetDndURI() const { return GetStringAt(ResultColumn::DND_URI); }
glib::HintsMap Result::GetHints() const
{
  glib::HintsMap hints;
  GetVariantAt(ResultColumn::METADATA).ASVToHints(hints);
  return hints;
}

LocalResult::LocalResult()
{
}

LocalResult::LocalResult(LocalResult const& other)
{
  operator=(other);
}

LocalResult::LocalResult(Result const& result)
{
  operator=(result);
}

LocalResult& LocalResult::operator=(Result const& rhs)
{
  uri = rhs.uri;
  icon_hint = rhs.icon_hint;
  category_index = rhs.category_index;
  result_type = rhs.result_type;
  mimetype = rhs.mimetype;
  name = rhs.name;
  comment = rhs.comment;
  dnd_uri = rhs.dnd_uri;  
  hints = rhs.hints;

  return *this;
}

LocalResult& LocalResult::operator=(LocalResult const& rhs)
{
  if (this == &rhs)
    return *this;

  uri = rhs.uri;
  icon_hint = rhs.icon_hint;
  category_index = rhs.category_index;
  result_type = rhs.result_type;
  mimetype = rhs.mimetype;
  name = rhs.name;
  comment = rhs.comment;
  dnd_uri = rhs.dnd_uri;
  hints = rhs.hints;

  return *this;
}

bool LocalResult::operator==(LocalResult const& rhs) const
{
  return uri == rhs.uri;
}

bool LocalResult::operator!=(LocalResult const& rhs) const
{
  return !(operator==(rhs));
}

std::vector<glib::Variant> LocalResult::Variants() const
{
  return {
    glib::Variant(uri), glib::Variant(icon_hint), glib::Variant(category_index),
    glib::Variant(result_type), glib::Variant(mimetype), glib::Variant(name),
    glib::Variant(comment), glib::Variant(dnd_uri), glib::Variant(hints)
  };
}

glib::Variant LocalResult::Variant() const
{
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("av"));

  std::vector<glib::Variant> vars = Variants();
  for (glib::Variant const& v : vars)
  {
    g_variant_builder_add(&b, "v", (GVariant*)v);
  }

  return g_variant_builder_end(&b);
}

LocalResult LocalResult::FromVariant(glib::Variant const& v)
{
  GVariantIter* var_iter;
  GVariant* value = NULL;

  if (!v || !g_variant_is_of_type (v, G_VARIANT_TYPE ("av")))
  {
    return LocalResult();
  }

  g_variant_get(v, g_variant_get_type_string(v), &var_iter);

  int i = 0;
  std::vector<glib::Variant> vars;
  LocalResult result;
  while (g_variant_iter_loop(var_iter, "v", &value))
  {
    vars.push_back(value);
    switch (i)
    {
      case URI:
        result.uri = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case ICON_HINT:
        result.icon_hint = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case CATEGORY:
        result.category_index = g_variant_get_uint32(value);
        break;
      case RESULT_TYPE:
        result.result_type = g_variant_get_uint32(value);
        break;
      case MIMETYPE:
        result.mimetype = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case TITLE:
        result.name = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case COMMENT:
        result.comment = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case DND_URI:
        result.dnd_uri = glib::gchar_to_string(g_variant_get_string(value, NULL));
        break;
      case METADATA:
        glib::HintsMap hints;
        if (glib::Variant(value).ASVToHints(hints))
        {
          result.hints = hints;
        }
        break;
    }
    i++;
  }
  g_variant_iter_free (var_iter);

  return result;
}

void LocalResult::clear()
{
  uri.clear();
}

bool LocalResult::empty() const
{
  return uri.empty();
}

}
}
