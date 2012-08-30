#include <gtest/gtest.h>
#include <glib-object.h>
#include <dee.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>
#include <UnityCore/HomeLens.h>
#include <UnityCore/Lens.h>
#include <UnityCore/Lenses.h>

#include "test_utils.h"

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace
{

/*
 * FORWARDS
 */

class StaticTestLens;

typedef struct {
  StaticTestLens* lens;
  gchar*          search_string;
} LensSearchClosure;

static gboolean dispatch_global_search(gpointer userdata);


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
class StaticTestLens : public Lens
{
public:
  typedef std::shared_ptr<StaticTestLens> Ptr;

  StaticTestLens(string const& id, string const& name, string const& description, string const& search_hint)
    : Lens(id, "", "", name, "lens-icon.png",
           description, search_hint, true, "",
           ModelType::LOCAL)
  {
    search_in_global(true);
    connected.SetGetterFunction(sigc::mem_fun(this, &StaticTestLens::force_connected));

    DeeModel* cats = categories()->model();
    DeeModel* results = global_results()->model();
    DeeModel* flters = filters()->model();

    // Set model schemas
    dee_model_set_schema(cats, "s", "s", "s", "a{sv}", NULL);
    dee_model_set_schema(results, "s", "s", "u", "s", "s", "s", "s", NULL);
    dee_model_set_schema(flters, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);

    // Populate categories model
    ostringstream cat0, cat1;
    cat0 << "cat0+" << id;
    cat1 << "cat1+" << id;
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    GVariant *asv = g_variant_builder_end(&b);

    dee_model_append(cats, cat0.str().c_str(), "icon.png", "tile-vertical", asv);
    dee_model_append(cats, cat1.str().c_str(), "icon.png", "tile-vertical", asv);
    dee_model_append(cats, "Shared cat", "icon.png", "tile-vertical", asv);
  }

  virtual ~StaticTestLens() {}

  bool force_connected()
  {
    return true;
  }

  virtual void DoGlobalSearch(string const& search_string)
  {
    DeeModel* model = global_results()->model();
    GVariant** row_buf = g_new(GVariant*, 8);

    row_buf[1] = g_variant_new_string("");
    row_buf[3] = g_variant_new_string("");
    row_buf[5] = g_variant_new_string("");
    row_buf[6] = g_variant_new_string("");
    row_buf[7] = NULL;

    unsigned int i;
    for (i = 0; i < search_string.size(); i++)
    {
      ostringstream uri;
      uri << "uri+" << search_string.at(i) << "+" << id();
      row_buf[0] = g_variant_new_string(uri.str().c_str());
      row_buf[2] = g_variant_new_uint32(i % 3);
      glib::String name(g_strdup_printf("%s - %d", search_string.c_str(), i));
      row_buf[4] = g_variant_new_string(name);

      dee_model_append_row(model, row_buf);
    }

    g_free(row_buf);

    global_search_finished.emit(Hints());
  }

  void GlobalSearch(string const& search_string)
  {
    /* Dispatch search async, because that's */
    LensSearchClosure* closure = g_new0(LensSearchClosure, 1);
    closure->lens = this;
    closure->search_string = g_strdup(search_string.c_str());
    g_idle_add(dispatch_global_search, closure);
  }

  void Search(string const& search_string)
  {

  }

  void Activate(string const& uri)
  {

  }

  void Preview(string const& uri)
  {

  }

};

static gboolean dispatch_global_search(gpointer userdata)
{
  LensSearchClosure* closure = (LensSearchClosure*) userdata;

  closure->lens->DoGlobalSearch(closure->search_string);

  g_free(closure->search_string);
  g_free(closure);

  return FALSE;
}

/*
 * Mock Lenses class
 */
class StaticTestLenses : public Lenses
{
public:
  typedef std::shared_ptr<StaticTestLenses> Ptr;

  StaticTestLenses()
  {
    count.SetGetterFunction(sigc::mem_fun(&list_, &Lenses::LensList::size));
  }

  virtual ~StaticTestLenses() {}

  Lenses::LensList GetLenses() const
  {
    return list_;
  }

  Lens::Ptr GetLens(std::string const& lens_id) const
  {
    for (auto lens : list_)
    {
      if (lens->id() == lens_id)
        return lens;
    }
    return Lens::Ptr();
  }

  Lens::Ptr GetLensAtIndex(std::size_t index) const
  {
    return list_.at(index);
  }

protected:
  Lenses::LensList list_;
};

class TwoStaticTestLenses : public StaticTestLenses
{
public:
  TwoStaticTestLenses()
  : lens_1_(new StaticTestLens("first.lens", "First Lens", "The very first lens", "First search hint"))
  , lens_2_(new StaticTestLens("second.lens", "Second Lens", "The second lens", "Second search hint"))
  {
    list_.push_back(lens_1_);
    list_.push_back(lens_2_);
  }

private:
  Lens::Ptr lens_1_;
  Lens::Ptr lens_2_;
};

class ThreeStaticTestLenses : public StaticTestLenses
{
public:
  ThreeStaticTestLenses()
  : lens_1_(new StaticTestLens("first.lens", "First Lens", "The very first lens", "First search hint"))
  , lens_2_(new StaticTestLens("second.lens", "Second Lens", "The second lens", "Second search hint"))
  , lens_3_(new StaticTestLens("applications.lens", "Applications", "The applications lens", "Search applications"))
  {
    list_.push_back(lens_1_);
    list_.push_back(lens_2_);
    list_.push_back(lens_3_);
  }

private:
  Lens::Ptr lens_1_;
  Lens::Ptr lens_2_;
  Lens::Ptr lens_3_;
};

TEST(TestHomeLens, TestConstruction)
{
  HomeLens home_lens_("name", "description", "searchhint");

  EXPECT_EQ(home_lens_.id(), "home.lens");
  EXPECT_EQ(home_lens_.connected, false);
  EXPECT_EQ(home_lens_.search_in_global, false);
  EXPECT_EQ(home_lens_.name, "name");
  EXPECT_EQ(home_lens_.description, "description");
  EXPECT_EQ(home_lens_.search_hint, "searchhint");
}

TEST(TestHomeLens, TestInitiallyEmpty)
{
  HomeLens home_lens_("name", "description", "searchhint");
  DeeModel* results = home_lens_.results()->model();
  DeeModel* categories = home_lens_.categories()->model();;
  DeeModel* filters = home_lens_.filters()->model();;

  EXPECT_EQ(dee_model_get_n_rows(results), 0);
  EXPECT_EQ(dee_model_get_n_rows(categories), 0);
  EXPECT_EQ(dee_model_get_n_rows(filters), 0);

  EXPECT_EQ(home_lens_.count(), 0);
}

TEST(TestHomeLens, TestTwoStaticLenses)
{
  HomeLens home_lens_("name", "description", "searchhint");
  TwoStaticTestLenses lenses_;

  home_lens_.AddLenses(lenses_);

  EXPECT_EQ(home_lens_.count, (size_t) 2);

  /* Test iteration of registered lensess */
  map<string,string> remaining;
  remaining["first.lens"] = "";
  remaining["second.lens"] = "";
  for (auto lens : home_lens_.GetLenses())
  {
    remaining.erase(lens->id());
  }

  EXPECT_EQ(remaining.size(), 0);

  /* Test sorting and GetAtIndex */
  EXPECT_EQ(home_lens_.GetLensAtIndex(0)->id(), "first.lens");
  EXPECT_EQ(home_lens_.GetLensAtIndex(1)->id(), "second.lens");
}

TEST(TestHomeLens, TestCategoryMergingDisplayName)
{
  HomeLens home_lens_("name", "description", "searchhint",
                      HomeLens::MergeMode::DISPLAY_NAME);
  TwoStaticTestLenses lenses_;
  DeeModel* cats = home_lens_.categories()->model();
  DeeModelIter* iter;
  unsigned int cat0_first = 0,
                cat1_first = 1,
                cat_shared = 2,
                cat0_second = 3,
                cat1_second = 4;
  const unsigned int NAME_COLUMN = 0;

  home_lens_.AddLenses(lenses_);

  EXPECT_EQ(dee_model_get_n_rows(cats), 5); // 5 because each lens has 3 cats, but 1 is shared between them

  /* Validate the merged categories */
  iter = dee_model_get_iter_at_row(cats, cat0_first);
  EXPECT_EQ("cat0+first.lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));

  iter = dee_model_get_iter_at_row(cats, cat1_first);
  EXPECT_EQ("cat1+first.lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));

  iter = dee_model_get_iter_at_row(cats, cat_shared);
  EXPECT_EQ("Shared cat", string(dee_model_get_string(cats, iter, NAME_COLUMN)));

  iter = dee_model_get_iter_at_row(cats, cat0_second);
  EXPECT_EQ("cat0+second.lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));

  iter = dee_model_get_iter_at_row(cats, cat1_second);
  EXPECT_EQ("cat1+second.lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));
}

TEST(TestHomeLens, TestCategoryMergingPerLens)
{
  HomeLens home_lens_("name", "description", "searchhint",
                      HomeLens::MergeMode::OWNER_LENS);
  TwoStaticTestLenses lenses_;
  DeeModel* cats = home_lens_.categories()->model();
  DeeModelIter* iter;
  const unsigned int NAME_COLUMN = 0;

  home_lens_.AddLenses(lenses_);

  EXPECT_EQ(dee_model_get_n_rows(cats), 2); // just two lenses

  /* Validate the merged categories */
  iter = dee_model_get_iter_at_row(cats, 0);
  EXPECT_EQ("First Lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));

  iter = dee_model_get_iter_at_row(cats, 1);
  EXPECT_EQ("Second Lens", string(dee_model_get_string(cats, iter, NAME_COLUMN)));
}

// It's not that we must not support filters. It is just not implemented yet.
// But we actively test against it to make sure we don't end up with broken
// filters in the UI. When/if we land support for filters on the home screen
// this test should obviously be removed
TEST(TestHomeLens, TestIgnoreFilters)
{
  HomeLens home_lens_("name", "description", "searchhint");
  TwoStaticTestLenses lenses_;
  DeeModel* filters = home_lens_.filters()->model();

  EXPECT_EQ(dee_model_get_n_rows(filters), 0);
}

TEST(TestHomeLens, TestOneSearch)
{
  HomeLens home_lens_("name", "description", "searchhint",
                      HomeLens::MergeMode::DISPLAY_NAME);
  TwoStaticTestLenses lenses_;
  DeeModel* results = home_lens_.results()->model();
  DeeModel* cats = home_lens_.categories()->model();
  DeeModel* filters = home_lens_.filters()->model();
  DeeModelIter* iter;
  unsigned int cat0_first = 0,
                cat1_first = 1,
                cat_shared = 2,
                cat0_second = 3,
                cat1_second = 4;
  const unsigned int URI_COLUMN = 0;
  const unsigned int CAT_COLUMN = 2;

  home_lens_.AddLenses(lenses_);

  home_lens_.Search("ape");

  bool finished = false;
  home_lens_.search_finished.connect([&finished] (Lens::Hints const& hints)
  {
    finished = true;
  });

  if (!finished)
    Utils::WaitUntil(finished);
  //Utils::WaitForTimeoutMSec();

  /* Validate counts */
  EXPECT_EQ(dee_model_get_n_rows(results), 6); // 3 hits from each lens
  EXPECT_EQ(dee_model_get_n_rows(cats), 5); // 5 because each lens has 3 cats, but 1 is shared between them
  EXPECT_EQ(dee_model_get_n_rows(filters), 0); // We ignore filters deliberately currently

  /* Validate results. In particular that we get the correct merged
   * category offsets assigned */
  iter = dee_model_get_iter_at_row(results, 0);
  EXPECT_EQ(string("uri+a+first.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat0_first, dee_model_get_uint32(results, iter, CAT_COLUMN));

  iter = dee_model_get_iter_at_row(results, 1);
  EXPECT_EQ(string("uri+p+first.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat1_first, dee_model_get_uint32(results, iter, CAT_COLUMN));

  iter = dee_model_get_iter_at_row(results, 2);
  EXPECT_EQ(string("uri+e+first.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat_shared, dee_model_get_uint32(results, iter, CAT_COLUMN));

  iter = dee_model_get_iter_at_row(results, 3);
  EXPECT_EQ(string("uri+a+second.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat0_second, dee_model_get_uint32(results, iter, CAT_COLUMN));

  iter = dee_model_get_iter_at_row(results, 4);
  EXPECT_EQ(string("uri+p+second.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat1_second, dee_model_get_uint32(results, iter, CAT_COLUMN));

  iter = dee_model_get_iter_at_row(results, 5);
  EXPECT_EQ(string("uri+e+second.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ(cat_shared, dee_model_get_uint32(results, iter, CAT_COLUMN));
}

TEST(TestHomeLens, TestOrderingAfterSearch)
{
  HomeLens home_lens_("name", "description", "searchhint",
                      HomeLens::MergeMode::OWNER_LENS);
  ThreeStaticTestLenses lenses_;
  DeeModel* results = home_lens_.results()->model();
  DeeModel* cats = home_lens_.categories()->model();
  DeeModel* filters = home_lens_.filters()->model();
  DeeModelIter* iter;
  // the lens is added as third, so must have cat == 2
  unsigned int apps_lens_cat = 2;
  const unsigned int URI_COLUMN = 0;
  const unsigned int CAT_COLUMN = 2;

  home_lens_.AddLenses(lenses_);

  home_lens_.Search("ape");

  bool finished = false;
  home_lens_.search_finished.connect([&finished] (Lens::Hints const& hints)
  {
    finished = true;
  });
  Utils::WaitUntil(finished);

  /* Validate counts */
  EXPECT_EQ(dee_model_get_n_rows(results), 9); // 3 hits from each lens
  EXPECT_EQ(dee_model_get_n_rows(cats), 3); // 3 cats since we are merging categories by lens
  EXPECT_EQ(dee_model_get_n_rows(filters), 0); // We ignore filters deliberately currently

  /* Validate the category order */
  auto order = home_lens_.GetCategoriesOrder();

  /* The home lens includes applications lens which contains exact match, must
   * be first category */
  EXPECT_EQ(order.at(0), apps_lens_cat);

  /* Validate results. In particular that we get the correct merged
   * category offsets assigned */
  iter = dee_model_get_iter_at_row(results, 0);
  EXPECT_EQ(string("uri+a+first.lens"), string(dee_model_get_string(results, iter, URI_COLUMN)));
  EXPECT_EQ( dee_model_get_uint32(results, iter, CAT_COLUMN), 0);
}

}
