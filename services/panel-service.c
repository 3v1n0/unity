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
  RE_SYNC,

  LAST_SIGNAL
};

enum
{
  SYNC_WAITING = -1,
  SYNC_NEUTRAL = 0,
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

  _service_signals[RE_SYNC] =
    g_signal_new ("re-sync",
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
  priv->timeouts[position] = SYNC_WAITING;

  g_signal_emit (self, _service_signals[RE_SYNC],
                 0, g_object_get_data (G_OBJECT (object), "id"));

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

  if (priv->timeouts[position] == SYNC_WAITING)
    {
      /* No need to ping again as we're waiting for the client to sync anyway */
      return;
    }
  else if (priv->timeouts[position] != SYNC_NEUTRAL)
    {
      /* We were going to signal that a sync was needed, but since there's been another change let's
       * hold off a little longer so we're not flooding the client
       */
      g_source_remove (priv->timeouts[position]);
    }

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

static gchar *
gtk_image_to_data (GtkImage *image)
{
  GtkImageType type = gtk_image_get_storage_type (image);
  gchar *ret = NULL;

  if (type == GTK_IMAGE_PIXBUF)
    {
      GdkPixbuf  *pixbuf;
      gchar      *buffer = NULL;
      gsize       buffer_size = 0;
      GError     *error = NULL;

      pixbuf = gtk_image_get_pixbuf (image);
     
      if (gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size, "png", &error, NULL))
        {
          ret = g_base64_encode ((const guchar *)buffer, buffer_size);
          g_free (buffer);
        }
      else
        {
          g_warning ("Unable to convert pixbuf to png data: '%s'", error ? error->message : "unknown");
          if (error)
            g_error_free (error);

          ret = g_strdup ("");
        }
    }
  else if (type == GTK_IMAGE_STOCK)
    {
      g_object_get (G_OBJECT (image), "stock", &ret, NULL);
    }
  else if (type == GTK_IMAGE_ICON_NAME)
    {
      g_object_get (G_OBJECT (image), "icon-name", &ret, NULL);
    }
  else if (type == GTK_IMAGE_GICON)
    {
      GIcon *icon = NULL;
      gtk_image_get_gicon (image, &icon, NULL);
      if (G_IS_ICON (icon))
        {
          ret = g_icon_to_string (icon);
        }
    }
  else
    {
      ret = g_strdup ("");
      g_warning ("Unable to support GtkImageType: %d", type);
    }

  return ret;
}

static void
indicator_entry_to_variant (IndicatorObjectEntry *entry,
                            const gchar          *id,
                            const gchar          *indicator_id,
                            GVariantBuilder      *b)
{
  gboolean is_label = GTK_IS_LABEL (entry->label);
  gboolean is_image = GTK_IS_IMAGE (entry->image);
  gchar *image_data = NULL;
  
  g_variant_builder_add (b, "(sssbbusbb)",
                         indicator_id,
                         id,
                         is_label ? gtk_label_get_label (entry->label) : "",
                         is_label ? gtk_widget_get_sensitive (GTK_WIDGET (entry->label)) : FALSE,
                         is_label ? gtk_widget_get_visible (GTK_WIDGET (entry->label)) : FALSE,
                         is_image ? (guint32)gtk_image_get_storage_type (entry->image) : (guint32) 0,
                         is_image ? (image_data = gtk_image_to_data (entry->image)) : "",
                         is_image ? gtk_widget_get_sensitive (GTK_WIDGET (entry->image)) : FALSE,
                         is_image ? gtk_widget_get_visible (GTK_WIDGET (entry->image)) : FALSE);

  g_free (image_data);
}

static void
indicator_object_to_variant (IndicatorObject *object, const gchar *indicator_id, GVariantBuilder *b)
{
  GList *entries, *e;

  entries = indicator_object_get_entries (object);
  for (e = entries; e; e = e->next)
    {
      IndicatorObjectEntry *entry = e->data;
      gchar *id = g_strdup_printf ("%p", entry);
      indicator_entry_to_variant (entry, id, indicator_id, b);
      g_free (id);
    }
  g_list_free (entries);
}

/*
 * Public Methods
 */
GVariant *
panel_service_sync (PanelService *self)
{
  GVariantBuilder b;
  GSList *i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(sssbbusbb))"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("a(sssbbusbb)"));

  for (i = self->priv->indicators; i; i = i->next)
    {
      const gchar *indicator_id = g_object_get_data (G_OBJECT (i->data), "id");
      gint position;

      /* Set the sync back to neutral */
      position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (i->data), "position"));
      self->priv->timeouts[position] = SYNC_NEUTRAL;
      
      indicator_object_to_variant (i->data, indicator_id, &b);
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}

GVariant *
panel_service_sync_one (PanelService *self, const gchar *indicator_id)
{
  GVariantBuilder b;
  GSList *i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(sssbbusbb))"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("a(sssbbusbb)"));

  for (i = self->priv->indicators; i; i = i->next)
    {
      if (g_strcmp0 (indicator_id,
                     g_object_get_data (G_OBJECT (i->data), "id")) == 0)
        {
          gint position;

          /* Set the sync back to neutral */
          position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (i->data), "position"));
          self->priv->timeouts[position] = SYNC_NEUTRAL;

          indicator_object_to_variant (i->data, indicator_id, &b);
          break;
        }
    }

  g_variant_builder_close (&b);
  return g_variant_builder_end (&b);
}
