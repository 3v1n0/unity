#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "panel-service.h"

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (PanelService, panel_service, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_SERVICE, PanelServicePrivate))

#define NOTIFY_TIMEOUT 80

struct _PanelServicePrivate
{
  GSList     *indicators;
  GHashTable *id2entry_hash;
  GHashTable *entry2indicator_hash;

  guint32 timeouts[100];
};

/* Globals */
enum
{
  ENTRY_ACTIVATED = 0,

  LAST_SIGNAL
};

static guint32 _service_signals[LAST_SIGNAL] = { 0 };

static gchar * indicator_order[] = {
  "libappmenu.so",
  "libapplication.so",
  "libnetwork.so",
  "libsoundmenu.so",
  "libmessaging.so",
  "libdatetime.so",
  "libme.so",
  "libsession.so",
  NULL
};

/* Forwards */
static void load_indicators (PanelService *self);
static void sort_indicators (PanelService *self);

/*
 * GObject stuff
 */
static void
panel_service_class_dispose (GObject *object)
{
  PanelServicePrivate *priv = PANEL_SERVICE (object)->priv;

  g_hash_table_destroy (priv->id2entry_hash);
  g_hash_table_destroy (priv->entry2indicator_hash);

  G_OBJECT_CLASS (panel_service_parent_class)->dispose (object);
}

static void
panel_service_class_init (PanelServiceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = panel_service_class_dispose;

  /* Signals */
  _service_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
  g_type_class_add_private (obj_class, sizeof (PanelServicePrivate));
}


static void
panel_service_init (PanelService *self)
{
  PanelServicePrivate *priv;
  priv = self->priv = GET_PRIVATE (self);

  priv->id2entry_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, NULL);
  priv->entry2indicator_hash = g_hash_table_new (g_direct_hash, g_direct_equal);

  load_indicators (self);
  sort_indicators (self);
}

PanelService *
panel_service_get_default ()
{
  static PanelService *service = NULL;
  
  if (service == NULL)
    service = g_object_new (PANEL_TYPE_SERVICE, NULL);

  return service;
}

/*
 * Private Methods
 */
static gboolean
actually_notify_object (IndicatorObject *object)
{
  PanelService *self;
  PanelServicePrivate *priv;
  gint position;

  self = panel_service_get_default ();
  priv = self->priv;

  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "position"));
  priv->timeouts[position] = 0;

  g_debug ("Notify object");
  return FALSE;
}

static void
notify_object (IndicatorObject *object)
{
  PanelService *self;
  PanelServicePrivate *priv;
  gint position;

  self = panel_service_get_default ();
  priv = self->priv;

  position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "position"));

  if (priv->timeouts[position])
      g_source_remove (priv->timeouts[position]);

  priv->timeouts[position] = g_timeout_add (NOTIFY_TIMEOUT,
                                            (GSourceFunc)actually_notify_object,
                                            object);
}

static void
on_entry_property_changed (GObject        *o,
                           GParamSpec      *pspec,
                           IndicatorObject *object)
{
  notify_object (object);
}

static void
on_entry_changed (GObject *o,
                  IndicatorObject *object)
{
  notify_object (object);
}

static void
on_entry_added (IndicatorObject      *object,
                IndicatorObjectEntry *entry,
                PanelService         *self)
{
  PanelServicePrivate *priv;
  gchar *id;

  g_return_if_fail (PANEL_IS_SERVICE (self));
  priv = self->priv;

  id = g_strdup_printf ("%p", entry);
  g_hash_table_insert (priv->id2entry_hash, id, entry);
  g_hash_table_insert (priv->entry2indicator_hash, entry, object);

  if (GTK_IS_LABEL (entry->label))
    {
      g_signal_connect (entry->label, "notify::label",
                        G_CALLBACK (on_entry_property_changed), object);

      g_signal_connect (entry->label, "notify::sensitive",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->label, "show",
                        G_CALLBACK (on_entry_changed), object);
      g_signal_connect (entry->label, "hide",
                        G_CALLBACK (on_entry_changed), object);
                        
    }
  if (GTK_IS_IMAGE (entry->image))
    {
      g_signal_connect (entry->image, "notify::storage-type",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::file",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::gicon",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::icon-name",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::pixbuf",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "notify::stock",
                        G_CALLBACK (on_entry_property_changed), object);

      g_signal_connect (entry->image, "notify::sensitive",
                        G_CALLBACK (on_entry_property_changed), object);
      g_signal_connect (entry->image, "show",
                        G_CALLBACK (on_entry_changed), object);
      g_signal_connect (entry->image, "hide",
                        G_CALLBACK (on_entry_changed), object);

    }
}

static void
on_entry_removed (IndicatorObject      *object,
                  IndicatorObjectEntry *entry,
                  PanelService         *self)
{

}

static void
load_indicators (PanelService *self)
{
  PanelServicePrivate *priv = self->priv;
  GDir        *dir;
  const gchar *name;

  if (!g_file_test (INDICATORDIR,
                    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    {
      g_warning ("%s does not exist, cannot read any indicators", INDICATORDIR);
      gtk_main_quit ();
    }

  dir = g_dir_open (INDICATORDIR, 0, NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      IndicatorObject *object;
      gchar           *path;
      GList           *entries, *entry;

      path = g_build_filename (INDICATORDIR, name, NULL);
      g_debug ("Loading: %s", path);

      object = indicator_object_new_from_file (path);
      if (object == NULL)
        {
          g_warning ("Unable to load '%s'", path);
          g_free (path);
          continue;
        }

      priv->indicators = g_slist_append (priv->indicators, object);

      g_object_set_data_full (G_OBJECT (object), "id", g_strdup (name), g_free);
      
      g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,
                        G_CALLBACK (on_entry_added), self);
      g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED,
                        G_CALLBACK (on_entry_removed), self);
      /* FIXME
      g_signal_connect (object, INDICATOR_OBJECT_SIGNAL_ENTRY_MOVED,
                        G_CALLBACK (on_entry_moved), service);*/

      entries = indicator_object_get_entries (object);
      for (entry = entries; entry != NULL; entry = entry->next)
        {
          on_entry_added (object, entry->data, self);
        }
      g_list_free (entries);
      g_free (path);
    }

  g_dir_close (dir);
}

static gint
name2order (const gchar * name)
{
	int i;

	for (i = 0; indicator_order[i] != NULL; i++)
    {
		  if (g_strcmp0(name, indicator_order[i]) == 0)
        {
		  	  return i;
    		}
	  }
	return -1;
}

static int
indicator_compare_func (IndicatorObject *o1, IndicatorObject *o2)
{
  gchar *s1;
  gchar *s2;
  int    i1;
  int    i2;

  s1 = g_object_get_data (G_OBJECT (o1), "id");
  s2 = g_object_get_data (G_OBJECT (o2), "id");
  
  i1 = name2order (s1);
  i2 = name2order (s2);

  return i1 - i2;
}

static void
sort_indicators (PanelService *self)
{
  GSList *i;
  int     k = 0;

  self->priv->indicators = g_slist_sort (self->priv->indicators,
                                         (GCompareFunc)indicator_compare_func);

  for (i = self->priv->indicators; i; i = i->next)
    {
      g_object_set_data (G_OBJECT (i->data), "position", GINT_TO_POINTER (k));
      k++;
    }
}

