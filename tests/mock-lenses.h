// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
*/

#ifndef UNITY_TESTS_MOCK_LENSES_H
#define UNITY_TESTS_MOCK_LENSES_H

#include <dee.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/Lens.h>
#include <UnityCore/Lenses.h>

namespace testmocks {

/*
 * FORWARDS
 */

class MockTestLens;

/*
 * Mock Lens instance that does not use DBus. The default search does like this:
 * For input "bar" output:
 *
 * i = 0
 * for letter in "bar":
 *   put result row [ "uri+$letter+$lens_id", "icon+$letter+$lens_id", i % 3, "mime+$letter+$lens_id", ...]
 *   i++
 *
 * The mock lens has 3 categories:
 *
 *  0) "cat0+$lens_id"
 *  1) "cat1+$lens_id"
 *  2) "Shared cat"
 */
class MockTestLens : public unity::dash::Lens
{
public:
  typedef std::shared_ptr<MockTestLens> Ptr;

  MockTestLens(std::string const& id, std::string const& name, std::string const& description, std::string const& search_hint)
    : Lens(id, "", "", name, "lens-icon.png",
           description, search_hint, true, "",
           unity::dash::ModelType::LOCAL)
    , num_results_(-1)
    , provides_personal_results_(true)
  {
    search_in_global(true);
    connected.SetGetterFunction(sigc::mem_fun(this, &MockTestLens::force_connected));
    provides_personal_content.SetGetterFunction(sigc::mem_fun(this, &MockTestLens::get_provides_personal_results));

    DeeModel* cats = categories()->model();
    DeeModel* results = global_results()->model();
    DeeModel* flters = filters()->model();

    // Set model schemas
    dee_model_set_schema(cats, "s", "s", "s", "a{sv}", NULL);
    dee_model_set_schema(results, "s", "s", "u", "s", "s", "s", "s", NULL);
    dee_model_set_schema(flters, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);

    // Populate categories model
    std::ostringstream cat0, cat1;
    cat0 << "cat0+" << id;
    cat1 << "cat1+" << id;
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    GVariant *asv = g_variant_builder_end(&b);

    dee_model_append(cats, cat0.str().c_str(), "icon.png", "tile-vertical", asv);
    dee_model_append(cats, cat1.str().c_str(), "icon.png", "tile-vertical", asv);
    dee_model_append(cats, "Shared cat", "icon.png", "tile-vertical", asv);
  }

  virtual ~MockTestLens() {}

  bool force_connected()
  {
    return true;
  }

  bool get_provides_personal_results()
  {
    return provides_personal_results_;
  }

  virtual void DoGlobalSearch(std::string const& search_string)
  {
    DeeModel* model = global_results()->model();
    GVariant** row_buf = g_new(GVariant*, 8);

    row_buf[1] = g_variant_new_string("");
    row_buf[3] = g_variant_new_string("");
    row_buf[5] = g_variant_new_string("");
    row_buf[6] = g_variant_new_string("");
    row_buf[7] = NULL;

    unsigned int i;
    unsigned int results_count = search_string.size();
    if (num_results_ >= 0) results_count = static_cast<unsigned>(num_results_);
    for (i = 0; i < results_count; i++)
    {
      std::ostringstream uri;
      char res_id(i >= search_string.size() ? '-' : search_string.at(i));
      uri << "uri+" << res_id << "+" << id();
      row_buf[0] = g_variant_new_string(uri.str().c_str());
      row_buf[2] = g_variant_new_uint32(i % 3);
      unity::glib::String name(g_strdup_printf("%s - %d",
            results_base_name_.empty() ?
                search_string.c_str() : results_base_name_.c_str(),
            i));
      row_buf[4] = g_variant_new_string(name);

      dee_model_append_row(model, row_buf);
    }

    g_free(row_buf);
  }

  void GlobalSearch(std::string const& search_string,
                    SearchFinishedCallback const& cb)
  {
    /* Dispatch search async, because that's what it'd normally do */
    source_manager_.Add(new unity::glib::Idle([this, search_string, cb] ()
    {
      DoGlobalSearch(search_string);
      cb(Lens::Hints(), unity::glib::Error());
      return false;
    }));
  }

  void Search(std::string const& search_string, SearchFinishedCallback const& cb)
  {
    /* Dispatch search async, because that's what it'd normally do */
    source_manager_.Add(new unity::glib::Idle([search_string, cb] ()
    {
      cb(Lens::Hints(), unity::glib::Error());
      return false;
    }));
  }

  void Activate(std::string const& uri)
  {

  }

  void Preview(std::string const& uri)
  {

  }

  void SetResultsBaseName(std::string const& name)
  {
    results_base_name_ = name;
  }

  void SetNumResults(int count)
  {
    num_results_ = count;
  }

  void SetProvidesPersonalResults(bool value)
  {
    provides_personal_results_ = value;
  }

private:
  std::string results_base_name_;
  int num_results_;
  bool provides_personal_results_;
  unity::glib::SourceManager source_manager_;
};

/*
 * Mock Lenses class
 */
class MockTestLenses : public unity::dash::Lenses
{
public:
  typedef std::shared_ptr<MockTestLenses> Ptr;

  MockTestLenses()
  {
    count.SetGetterFunction(sigc::mem_fun(&list_, &unity::dash::Lenses::LensList::size));
  }

  virtual ~MockTestLenses() {}

  unity::dash::Lenses::LensList GetLenses() const
  {
    return list_;
  }

  unity::dash::Lens::Ptr GetLens(std::string const& lens_id) const
  {
    for (auto lens : list_)
    {
      if (lens->id() == lens_id)
        return lens;
    }
    return unity::dash::Lens::Ptr();
  }

  unity::dash::Lens::Ptr GetLensAtIndex(std::size_t index) const
  {
    return list_.at(index);
  }

  unity::dash::Lens::Ptr GetLensForShortcut(std::string const& lens_shortcut) const override
  {
    return unity::dash::Lens::Ptr();
  }


protected:
  unity::dash::Lenses::LensList list_;
};

class TwoMockTestLenses : public MockTestLenses
{
public:
  TwoMockTestLenses()
  : lens_1_(new MockTestLens("first.lens", "First Lens", "The very first lens", "First search hint"))
  , lens_2_(new MockTestLens("second.lens", "Second Lens", "The second lens", "Second search hint"))
  {
    list_.push_back(lens_1_);
    list_.push_back(lens_2_);
  }

private:
  unity::dash::Lens::Ptr lens_1_;
  unity::dash::Lens::Ptr lens_2_;
};

class ThreeMockTestLenses : public MockTestLenses
{
public:
  ThreeMockTestLenses()
  : lens_1_(new MockTestLens("first.lens", "First Lens", "The very first lens", "First search hint"))
  , lens_2_(new MockTestLens("second.lens", "Second Lens", "The second lens", "Second search hint"))
  , lens_3_(new MockTestLens("applications.lens", "Applications", "The applications lens", "Search applications"))
  {
    list_.push_back(lens_1_);
    list_.push_back(lens_2_);
    list_.push_back(lens_3_);
  }

private:
  unity::dash::Lens::Ptr lens_1_;
  unity::dash::Lens::Ptr lens_2_;
  unity::dash::Lens::Ptr lens_3_;
};

}

#endif
