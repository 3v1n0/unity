#include <gtest/gtest.h>
#include <glib-object.h>
#include <dee.h>
#include <string>
#include <stdexcept>
#include <map>
#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/HomeLens.h>
#include <UnityCore/Lens.h>
#include <UnityCore/Lenses.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

/*
 * Mock Lens instance that does not use DBus
 */
class StaticTestLens : public Lens
{
public:
  typedef std::shared_ptr<StaticTestLens> Ptr;

  StaticTestLens(string const& id, string const& name, string const& description, string const& search_hint)
    : Lens(id, "", "", name, "lens-icon.png",
           description, search_hint, true, "",
           ModelType::LOCAL) {}

  virtual ~StaticTestLens() {}

  void GlobalSearch(std::string const& search_string)
  {

  }

  void Search(std::string const& search_string)
  {

  }

  void Activate(std::string const& uri)
  {

  }

  void Preview(std::string const& uri)
  {

  }
};

/*
 * Mock Lenses class that comes with 2 pre-allocated lenses
 */
class StaticTestLenses : public Lenses
{
public:
  typedef std::shared_ptr<StaticTestLenses> Ptr;

  StaticTestLenses()
    : lens_1_(new StaticTestLens("first.lens", "First Lens", "The very first lens", "First search hint"))
    , lens_2_(new StaticTestLens("second.lens", "Second Lens", "The second lens", "Second search hint"))
  {
    count.SetGetterFunction(sigc::mem_fun(&list_, &Lenses::LensList::size));
    list_.push_back(lens_1_);
    list_.push_back(lens_2_);
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

private:
  Lenses::LensList list_;
  Lens::Ptr lens_1_;
  Lens::Ptr lens_2_;
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
  StaticTestLenses lenses_;

  home_lens_.AddLenses(lenses_);

  EXPECT_EQ(home_lens_.count, (size_t) 2);
}

}
