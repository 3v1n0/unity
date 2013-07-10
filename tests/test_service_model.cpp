#include "test_service_model.h"
#include <dee.h>
#include "config.h"

namespace unity
{
namespace service
{

Model::Model()
  : model_(dee_shared_model_new("com.canonical.test.model"))
  , results_model_(dee_shared_model_new("com.canonical.test.resultsmodel"))
  , categories_model_(dee_shared_model_new("com.canonical.test.categoriesmodel"))
  , categories_changing_model_(dee_shared_model_new("com.canonical.test.categoriesmodel_changing"))
  , tracks_model_(dee_shared_model_new("com.canonical.test.tracks"))
{
  PopulateTestModel();
  PopulateResultsModel();
  PopulateCategoriesModel();
  PopulateCategoriesChangesModel();
  PopulateTracksModel();
}

void Model::PopulateTestModel()
{
  dee_model_set_schema(model_, "u", "s", nullptr);

  for (unsigned i = 0; i < 100; ++i)
    dee_model_append(model_, i, ("Test"+std::to_string(i)).c_str());
}

void Model::PopulateResultsModel()
{
  dee_model_set_schema(results_model_, "s", "s", "u", "u", "s", "s", "s", "s", "a{sv}", nullptr);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "key", g_variant_new_string("value"));
  GVariant *hints = g_variant_builder_end(&b);
  g_variant_ref_sink(hints);

  for(unsigned i = 0; i < 200; ++i)
  {
    gchar* name = g_strdup_printf("Result%u", i);
    dee_model_append(results_model_,
                     name,
                     name,
                     i/50, // new category every 50 results
                     0,
                     name,
                     name,
                     name,
                     name,
                     hints);
    g_free(name);
  }
  g_variant_unref(hints);
}

void Model::PopulateCategoriesModel()
{
  dee_model_set_schema(categories_model_, "s", "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);
  g_variant_ref_sink(hints);

  guint i;
  for(i = 0; i < 5; i++)
  {
    gchar* id = g_strdup_printf("cat%u", i);
    gchar* name = g_strdup_printf("Category %u", i);
    dee_model_append(categories_model_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
  }
  g_variant_unref(hints);
}

void Model::PopulateCategoriesChangesModel()
{
  dee_model_set_schema(categories_changing_model_, "s", "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);
  g_variant_ref_sink(hints);

  guint i;
  for(i = 0; i < 5; i++)
  {
    gchar* id = g_strdup_printf("cat%u", i);
    gchar* name = g_strdup_printf("Category %u", i);
    dee_model_append(categories_changing_model_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
  }
  g_variant_unref(hints);

  category_timeout_.reset(new glib::Timeout(200, sigc::mem_fun(this, &Model::OnCategoryChangeTimeout)));
}

bool Model::OnCategoryChangeTimeout()
{
  DeeModelIter* iter = dee_model_get_first_iter(categories_changing_model_);
  if (!iter)
    return TRUE;

  static guint new_category = 5;

  // remove first row.
  dee_model_remove(categories_changing_model_, iter);

  // change first
  iter = dee_model_get_first_iter(categories_changing_model_);
  if (iter)
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    GVariant *hints = g_variant_builder_end(&b);
    g_variant_ref_sink(hints);

    gchar* id = g_strdup_printf("cat%u", new_category);
    gchar* name = g_strdup_printf("Category %u", new_category);
    dee_model_set(categories_changing_model_,
                  iter,
                  id,
                  name,
                  "gtk-apply",
                  "grid",
                  hints);
    g_free(id);
    g_free(name);
    g_variant_unref(hints);

    new_category++;
  }

  // append new,
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    GVariant *hints = g_variant_builder_end(&b);
    g_variant_ref_sink(hints);

    gchar* id = g_strdup_printf("cat%u", new_category);
    gchar* name = g_strdup_printf("Category %u", new_category);
    dee_model_append(categories_changing_model_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
    g_variant_unref(hints);

    new_category++;
  }

  return TRUE;
}

void Model::PopulateTracksModel()
{
  dee_model_set_schema(tracks_model_, "s", "i", "s", "u", NULL);

  guint i;
  for(i = 0; i < 5; i++)
  {
    gchar* uri = g_strdup_printf("uri://track%u", i);
    gchar* title = g_strdup_printf("Track %u", i);

    dee_model_append(tracks_model_,
                     uri,
                     i+1,
                     title,
                     (unsigned)10);
    g_free(uri);
    g_free(title);
  }

  track_timeout_.reset(new glib::Timeout(200, sigc::mem_fun(this, &Model::OnTrackChangeTimeout)));
}

bool Model::OnTrackChangeTimeout()
{
  DeeModelIter* iter = dee_model_get_first_iter(tracks_model_);
  if (!iter)
    return TRUE;

  static int new_track = 5;

  // remove first row.
  dee_model_remove(tracks_model_, iter);

  // change first
  iter = dee_model_get_first_iter(tracks_model_);
  if (iter)
  {
    gchar* uri = g_strdup_printf("uri://track%u", new_track);
    gchar* title = g_strdup_printf("Track %u", new_track);

    dee_model_set(tracks_model_,
                  iter,
                  uri,
                  new_track+1,
                  title,
                  (unsigned)10);
    g_free(uri);
    g_free(title);

    new_track++;
  }

  // append new,
  {
    gchar* uri = g_strdup_printf("uri://track%u", new_track);
    gchar* title = g_strdup_printf("Track %u", new_track);

    dee_model_append(tracks_model_,
                     uri,
                     new_track+1,
                     title,
                     (unsigned)10);
    g_free(uri);
    g_free(title);

    new_track++;
  }

  return TRUE;
}


}
}
