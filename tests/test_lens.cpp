#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Lens.h>
#include <UnityCore/MusicPreviews.h>

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
  {
    WaitForConnected();

    Categories::Ptr categories = lens_->categories;
    categories->category_added.connect(sigc::mem_fun(this, &TestLens::OnCategoryAdded));
  }

  void OnCategoryAdded(Category const& category)
  {
    n_categories_++;
  }

  static gboolean TimeoutCallback(gpointer data)
  {
    *(bool*)data = true;
    return FALSE;
  };

  guint32 ScheduleTimeout(bool* timeout_reached)
  {
    return g_timeout_add_seconds(10, TimeoutCallback, timeout_reached);
  }

  void WaitUntil(bool& success)
  {
    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached);

    while (!success && !timeout_reached)
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

    if (success)
      g_source_remove(timeout_id);

    EXPECT_TRUE(success);
  }
 
  void WaitForConnected()
  {
    bool timeout_reached = false;
    guint32 timeout_id = ScheduleTimeout(&timeout_reached);

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
    guint32 timeout_id = ScheduleTimeout(&timeout_reached);
    
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
  lens_->activated.connect(sigc::slot<void, std::string const&, HandledType,Lens::Hints const&>(activated_cb));  
  
  lens_->Activate(uri);
  WaitUntil(activated);
}

TEST_F(TestLens, TestPreview)
{
  std::string uri = PopulateAndGetFirstResultURI();
  bool previewed = false;

  auto preview_cb = [&previewed, &uri] (std::string const& uri_,
                                  Preview::Ptr preview)
  {
    EXPECT_EQ(uri, uri_);
    EXPECT_EQ(preview->renderer_name, "preview-track");

    TrackPreview::Ptr track_preview = std::static_pointer_cast<TrackPreview>(preview);
    EXPECT_EQ(track_preview->number, (unsigned int)1);
    EXPECT_EQ(track_preview->title, "Animus Vox");
    EXPECT_EQ(track_preview->artist, "The Glitch Mob");
    EXPECT_EQ(track_preview->album, "Drink The Sea");
    EXPECT_EQ(track_preview->length, (unsigned int)404);
    EXPECT_EQ(track_preview->album_cover, "file://music/the/track");
    EXPECT_EQ(track_preview->primary_action_name, "Play");
    EXPECT_EQ(track_preview->primary_action_icon_hint, "");
    EXPECT_EQ(track_preview->primary_action_uri, "play://music/the/track");
    EXPECT_EQ(track_preview->play_action_uri, "preview://music/the/track");
    EXPECT_EQ(track_preview->pause_action_uri, "pause://music/the/track");

    previewed = true;
  };
  lens_->preview_ready.connect(sigc::slot<void, std::string const&, Preview::Ptr>(preview_cb));

  lens_->Preview(uri);
  WaitUntil(previewed);
}

}
