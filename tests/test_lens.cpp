#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/CheckOptionFilter.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Lens.h>
#include <UnityCore/MultiRangeFilter.h>
#include <UnityCore/Preview.h>
#include <UnityCore/MoviePreview.h>
#include <UnityCore/Variant.h>
#include <UnityCore/RadioOptionFilter.h>
#include <UnityCore/RatingsFilter.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

static const string lens_name = "com.canonical.Unity.Test";
static const string lens_path = "/com/canonical/unity/testlens";

class TestLens : public ::testing::Test
{
public:
  TestLens()
    : lens_(new Lens("testlens", lens_name, lens_path,
                     "Test Lens", "gtk-apply")),
      n_categories_(0)
    , n_filters_(0)
  {
    WaitForConnected();

    Categories::Ptr categories = lens_->categories;
    categories->category_added.connect(sigc::mem_fun(this, &TestLens::OnCategoryAdded));

    Filters::Ptr filters = lens_->filters;
    filters->filter_added.connect(sigc::mem_fun(this, &TestLens::OnFilterAdded));
  }

  void OnCategoryAdded(Category const& category)
  {
    n_categories_++;
  }

  void OnFilterAdded(Filter::Ptr filter)
  {
    n_filters_++;
  }
 
  void WaitForConnected()
  {
    bool timeout_reached = false;
    guint32 timeout_id = Utils::ScheduleTimeout(&timeout_reached, 2000);

    while (!lens_->connected && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (lens_->connected)
      g_source_remove(timeout_id);

    EXPECT_FALSE(timeout_reached);
  }

  template<typename Adaptor>
  void WaitForModel(Model<Adaptor>* model, unsigned int n_rows)
  {
    bool timeout_reached = false;
    guint32 timeout_id = Utils::ScheduleTimeout(&timeout_reached, 2000);
    
    while (model->count != n_rows && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (model->count == n_rows)
      g_source_remove(timeout_id);

    EXPECT_FALSE(timeout_reached);
  }

  std::string PopulateAndGetFirstResultURI()
  {
    Results::Ptr results = lens_->results;

    lens_->Search("Test Search String");
    WaitForModel<Result>(results.get(), 5);
    
    Result result = results->RowAtIndex(0);
    return result.uri;
  }

  Lens::Ptr lens_;
  unsigned int n_categories_;
  unsigned int n_filters_;
};

TEST_F(TestLens, TestConnection)
{
  EXPECT_EQ(lens_->id, "testlens");
  EXPECT_EQ(lens_->dbus_name, lens_name);
  EXPECT_EQ(lens_->dbus_path, lens_path);
  EXPECT_EQ(lens_->name, "Test Lens");
  EXPECT_EQ(lens_->icon_hint, "gtk-apply");
}

TEST_F(TestLens, TestSynchronization)
{
  EXPECT_EQ(lens_->search_hint, "Search Test Lens");
  EXPECT_TRUE(lens_->visible);
  EXPECT_TRUE(lens_->search_in_global);
}

TEST_F(TestLens, TestCategories)
{
  Categories::Ptr categories = lens_->categories;
  WaitForModel<Category>(categories.get(), 3);

  EXPECT_EQ(categories->count, (unsigned int)3);
  EXPECT_EQ(n_categories_, 3);
  
  Category category = categories->RowAtIndex(0);
  EXPECT_EQ(category.name, "Category1");
  EXPECT_EQ(category.icon_hint, "gtk-apply");
  EXPECT_EQ(category.index, (unsigned int)0);
  EXPECT_EQ(category.renderer_name, "tile-vertical");

  category = categories->RowAtIndex(1);
  EXPECT_EQ(category.name, "Category2");
  EXPECT_EQ(category.icon_hint, "gtk-cancel");
  EXPECT_EQ(category.index, (unsigned int)1);
  EXPECT_EQ(category.renderer_name, "tile-horizontal");

  category = categories->RowAtIndex(2);
  EXPECT_EQ(category.name, "Category3");
  EXPECT_EQ(category.icon_hint, "gtk-close");
  EXPECT_EQ(category.index, (unsigned int)2);
  EXPECT_EQ(category.renderer_name, "flow");
}

TEST_F(TestLens, TestSearch)
{
  Results::Ptr results = lens_->results;

  lens_->Search("Test Search String");
  WaitForModel<Result>(results.get(), 5);

  for (unsigned int i = 0; i < 5; i++)
  {
    std::string name("Test Search String" + boost::lexical_cast<std::string>(i));

    Result result = results->RowAtIndex(i);
    
    std::string uri = result.uri;
    EXPECT_TRUE(uri.find("file:///test"));
    EXPECT_EQ(result.icon_hint, "gtk-apply");
    EXPECT_EQ(result.category_index, i);
    EXPECT_EQ(result.mimetype, "text/html");
    EXPECT_EQ(result.name, name);
    EXPECT_EQ(result.comment, "kamstrup likes ponies");
    EXPECT_EQ(result.dnd_uri, "file:///test");
  }
}

TEST_F(TestLens, TestGlobalSearch)
{
  Results::Ptr results = lens_->global_results;

  lens_->GlobalSearch("Test Global Search String");
  WaitForModel<Result>(results.get(), 10);

  for (unsigned int i = 0; i < 10; i++)
  {
    std::string name("Test Global Search String" + boost::lexical_cast<std::string>(i));

    Result result = results->RowAtIndex(i);

    std::string uri = result.uri;
    EXPECT_TRUE(uri.find("file:///test"));
    EXPECT_EQ(result.icon_hint, "gtk-apply");
    EXPECT_EQ(result.category_index, i);
    EXPECT_EQ(result.mimetype, "text/html");
    EXPECT_EQ(result.name, name);
    EXPECT_EQ(result.comment, "kamstrup likes ponies");
    EXPECT_EQ(result.dnd_uri, "file:///test");
  }
}

TEST_F(TestLens, TestActivation)
{
  std::string uri = PopulateAndGetFirstResultURI();

  bool activated = false;
  auto activated_cb = [&activated, &uri] (std::string const& uri_,
                                          HandledType handled,
                                          Lens::Hints const& hints)
  {
    EXPECT_EQ(uri, uri_);
    EXPECT_EQ(handled, HandledType::HIDE_DASH);
    EXPECT_EQ(hints.size(), 0);
    activated = true;
  };
  lens_->activated.connect(activated_cb);

  lens_->Activate(uri);
  Utils::WaitUntil(activated);
}

TEST_F(TestLens, TestPreview)
{
  std::string uri = PopulateAndGetFirstResultURI();
  bool previewed = false;

  auto preview_cb = [&previewed, &uri] (std::string const& uri_,
                                        Preview::Ptr const& preview)
  {
    EXPECT_EQ(uri, uri_);
    EXPECT_EQ(preview->renderer_name, "preview-movie");

    auto movie_preview = std::dynamic_pointer_cast<MoviePreview>(preview);
    EXPECT_EQ(movie_preview->title, "A movie");
    EXPECT_EQ(movie_preview->subtitle, "With subtitle");
    EXPECT_EQ(movie_preview->description, "And description");

    previewed = true;
  };

  lens_->preview_ready.connect(preview_cb);

  lens_->Preview(uri);
  Utils::WaitUntil(previewed);
}

TEST_F(TestLens, TestPreviewAction)
{
  std::string uri = PopulateAndGetFirstResultURI();
  bool previewed = false;
  Preview::Ptr preview;

  auto preview_cb = [&previewed, &uri, &preview]
                                        (std::string const& uri_,
                                         Preview::Ptr const& preview_)
  {
    EXPECT_EQ(uri, uri_);
    EXPECT_EQ(preview_->renderer_name, "preview-movie");

    preview = preview_;
    previewed = true;
  };

  lens_->preview_ready.connect(preview_cb);
  lens_->Preview(uri);

  Utils::WaitUntil(previewed);

  bool action_executed = false;
  auto activated_cb = [&action_executed] (std::string const& uri,
                                          HandledType handled_type,
                                          Lens::Hints const& hints)
  {
    EXPECT_EQ(handled_type, HandledType::SHOW_DASH);
    action_executed = true;
  };

  lens_->activated.connect(activated_cb);
  EXPECT_GT(preview->GetActions().size(), (unsigned)0);
  auto action = preview->GetActions()[0];
  preview->PerformAction(action->id);

  Utils::WaitUntil(action_executed);
}

TEST_F(TestLens, TestPreviewClosedSignal)
{
  std::string uri = PopulateAndGetFirstResultURI();
  bool previewed = false;
  MoviePreview::Ptr movie_preview;

  auto preview_cb = [&previewed, &uri, &movie_preview]
                                        (std::string const& uri_,
                                         Preview::Ptr const& preview)
  {
    EXPECT_EQ(uri, uri_);
    EXPECT_EQ(preview->renderer_name, "preview-movie");

    movie_preview = std::dynamic_pointer_cast<MoviePreview>(preview);

    //there isn't anything we can really test here - other than it's not crashing here
    movie_preview->PreviewClosed();

    previewed = true;
  };

  lens_->preview_ready.connect(preview_cb);
  lens_->Preview(uri);

  Utils::WaitUntil(previewed);
}

TEST_F(TestLens, TestFilterSync)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  EXPECT_EQ(filters->count, (unsigned int)4);
  EXPECT_EQ(n_filters_, 4);

}

TEST_F(TestLens, TestFilterRadioOption)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  RadioOptionFilter::Ptr filter = static_pointer_cast<RadioOptionFilter>(filters->FilterAtIndex(0));
  EXPECT_EQ(filter->id, "when");
  EXPECT_EQ(filter->name, "When");
  EXPECT_EQ(filter->icon_hint, "");
  EXPECT_EQ(filter->renderer_name, "filter-radiooption");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  RadioOptionFilter::RadioOptions options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)3);
  
  EXPECT_EQ(options[0]->id, "today");
  EXPECT_EQ(options[0]->name, "Today");
  EXPECT_EQ(options[0]->icon_hint, "");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "yesterday");
  EXPECT_EQ(options[1]->name, "Yesterday");
  EXPECT_EQ(options[1]->icon_hint, "");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "lastweek");
  EXPECT_EQ(options[2]->name, "Last Week");
  EXPECT_EQ(options[2]->icon_hint, "");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestLens, TestFilterRadioOptionLogic)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  RadioOptionFilter::Ptr filter = static_pointer_cast<RadioOptionFilter>(filters->FilterAtIndex(0));
  RadioOptionFilter::RadioOptions options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  options[0]->active = false;
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[1]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[2]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_TRUE (options[2]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
}

TEST_F(TestLens, TestFilterCheckOption)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  CheckOptionFilter::Ptr filter = static_pointer_cast<CheckOptionFilter>(filters->FilterAtIndex(1));
  EXPECT_EQ(filter->id, "type");
  EXPECT_EQ(filter->name, "Type");
  EXPECT_EQ(filter->icon_hint, "");
  EXPECT_EQ(filter->renderer_name, "filter-checkoption");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  CheckOptionFilter::CheckOptions options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)3);
  
  EXPECT_EQ(options[0]->id, "apps");
  EXPECT_EQ(options[0]->name, "Apps");
  EXPECT_EQ(options[0]->icon_hint, "gtk-apps");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "files");
  EXPECT_EQ(options[1]->name, "Files");
  EXPECT_EQ(options[1]->icon_hint, "gtk-files");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "music");
  EXPECT_EQ(options[2]->name, "Music");
  EXPECT_EQ(options[2]->icon_hint, "gtk-music");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestLens, TestFilterCheckOptionLogic)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  CheckOptionFilter::Ptr filter = static_pointer_cast<CheckOptionFilter>(filters->FilterAtIndex(1));
  CheckOptionFilter::CheckOptions options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  options[0]->active = false;
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[1]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[2]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_TRUE (options[2]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
}

TEST_F(TestLens, TestFilterRatings)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  RatingsFilter::Ptr filter = static_pointer_cast<RatingsFilter>(filters->FilterAtIndex(2));
  EXPECT_EQ(filter->id, "ratings");
  EXPECT_EQ(filter->name, "Ratings");
  EXPECT_EQ(filter->icon_hint, "");
  std::string tmp = filter->renderer_name;
  EXPECT_EQ(filter->renderer_name, "filter-ratings");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  EXPECT_FLOAT_EQ(filter->rating, 0.0f);
  filter->rating = 0.5f;
  EXPECT_FLOAT_EQ(filter->rating, 0.5f);
}

TEST_F(TestLens, TestFilterMultiRange)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  MultiRangeFilter::Ptr filter = static_pointer_cast<MultiRangeFilter>(filters->FilterAtIndex(3));
  EXPECT_EQ(filter->id, "size");
  EXPECT_EQ(filter->name, "Size");
  EXPECT_EQ(filter->icon_hint, "");
  std::string tmp = filter->renderer_name;
  EXPECT_EQ(filter->renderer_name, "filter-multirange");
  EXPECT_TRUE(filter->visible);
  EXPECT_TRUE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  MultiRangeFilter::Options options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)4);
  
  EXPECT_EQ(options[0]->id, "1MB");
  EXPECT_EQ(options[0]->name, "1MB");
  EXPECT_EQ(options[0]->icon_hint, "");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "10MB");
  EXPECT_EQ(options[1]->name, "10MB");
  EXPECT_EQ(options[1]->icon_hint, "");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "100MB");
  EXPECT_EQ(options[2]->name, "100MB");
  EXPECT_EQ(options[2]->icon_hint, "");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestLens, TestFilterMultiRangeLogic)
{
  Filters::Ptr filters = lens_->filters;
  WaitForModel<FilterAdaptor>(filters.get(), 4);

  MultiRangeFilter::Ptr filter = static_pointer_cast<MultiRangeFilter>(filters->FilterAtIndex(3));
  MultiRangeFilter::Options options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  options[0]->active = true;
  options[3]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_TRUE (options[2]->active);
  EXPECT_TRUE (options[3]->active);

  options[0]->active = true;
  options[2]->active = false;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_TRUE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  options[0]->active = false;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_TRUE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);
}


}
