/* panel-window-buttons.c generated by valac 0.9.8, the Vala compiler
 * generated from panel-window-buttons.vala, do not modify */

/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <clutk/clutk.h>
#include <libbamf/libbamf.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <clutter/clutter.h>
#include <gio/gio.h>
#include <float.h>
#include <math.h>
#include <glib/gstdio.h>


#define UNITY_PANEL_TYPE_WINDOW_BUTTONS (unity_panel_window_buttons_get_type ())
#define UNITY_PANEL_WINDOW_BUTTONS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTONS, UnityPanelWindowButtons))
#define UNITY_PANEL_WINDOW_BUTTONS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PANEL_TYPE_WINDOW_BUTTONS, UnityPanelWindowButtonsClass))
#define UNITY_PANEL_IS_WINDOW_BUTTONS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTONS))
#define UNITY_PANEL_IS_WINDOW_BUTTONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PANEL_TYPE_WINDOW_BUTTONS))
#define UNITY_PANEL_WINDOW_BUTTONS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTONS, UnityPanelWindowButtonsClass))

typedef struct _UnityPanelWindowButtons UnityPanelWindowButtons;
typedef struct _UnityPanelWindowButtonsClass UnityPanelWindowButtonsClass;
typedef struct _UnityPanelWindowButtonsPrivate UnityPanelWindowButtonsPrivate;

#define UNITY_PANEL_TYPE_WINDOW_BUTTON (unity_panel_window_button_get_type ())
#define UNITY_PANEL_WINDOW_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTON, UnityPanelWindowButton))
#define UNITY_PANEL_WINDOW_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PANEL_TYPE_WINDOW_BUTTON, UnityPanelWindowButtonClass))
#define UNITY_PANEL_IS_WINDOW_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTON))
#define UNITY_PANEL_IS_WINDOW_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PANEL_TYPE_WINDOW_BUTTON))
#define UNITY_PANEL_WINDOW_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PANEL_TYPE_WINDOW_BUTTON, UnityPanelWindowButtonClass))

typedef struct _UnityPanelWindowButton UnityPanelWindowButton;
typedef struct _UnityPanelWindowButtonClass UnityPanelWindowButtonClass;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))
typedef struct _UnityPanelWindowButtonPrivate UnityPanelWindowButtonPrivate;
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

struct _UnityPanelWindowButtons {
	CtkBox parent_instance;
	UnityPanelWindowButtonsPrivate * priv;
};

struct _UnityPanelWindowButtonsClass {
	CtkBoxClass parent_class;
};

struct _UnityPanelWindowButtonsPrivate {
	CtkText* appname;
	UnityPanelWindowButton* close;
	UnityPanelWindowButton* minimize;
	UnityPanelWindowButton* unmaximize;
	BamfMatcher* matcher;
	UnityAppInfoManager* appinfo;
	guint32 last_xid;
	BamfView* _last_view;
};

struct _UnityPanelWindowButton {
	CtkButton parent_instance;
	UnityPanelWindowButtonPrivate * priv;
	ClutterActor* bg;
};

struct _UnityPanelWindowButtonClass {
	CtkButtonClass parent_class;
};

struct _UnityPanelWindowButtonPrivate {
	char* _filename;
	gboolean using_beta;
	gint icon_size;
	char* directory;
};


static gpointer unity_panel_window_buttons_parent_class = NULL;
static gpointer unity_panel_window_button_parent_class = NULL;

GType unity_panel_window_buttons_get_type (void) G_GNUC_CONST;
GType unity_panel_window_button_get_type (void) G_GNUC_CONST;
#define UNITY_PANEL_WINDOW_BUTTONS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UNITY_PANEL_TYPE_WINDOW_BUTTONS, UnityPanelWindowButtonsPrivate))
enum  {
	UNITY_PANEL_WINDOW_BUTTONS_DUMMY_PROPERTY
};
#define UNITY_PANEL_WINDOW_BUTTONS_FORMAT "<b>%s</b>"
UnityPanelWindowButtons* unity_panel_window_buttons_new (void);
UnityPanelWindowButtons* unity_panel_window_buttons_construct (GType object_type);
static void unity_panel_window_buttons_on_active_window_changed (UnityPanelWindowButtons* self, GObject* object, GObject* object1);
static void unity_panel_window_buttons_on_last_view_destroyed (UnityPanelWindowButtons* self, GObject* object);
static void _unity_panel_window_buttons_on_last_view_destroyed_gweak_notify (gpointer self, GObject* object);
static void unity_panel_window_buttons_real_get_preferred_width (ClutterActor* base, float for_height, float* min_width, float* nat_width);
UnityPanelWindowButton* unity_panel_window_button_new (const char* filename);
UnityPanelWindowButton* unity_panel_window_button_construct (GType object_type, const char* filename);
static void _lambda9_ (UnityPanelWindowButtons* self);
static void __lambda9__ctk_button_clicked (CtkButton* _sender, gpointer self);
static void _lambda10_ (UnityPanelWindowButtons* self);
static void __lambda10__ctk_button_clicked (CtkButton* _sender, gpointer self);
static void _lambda11_ (UnityPanelWindowButtons* self);
static void __lambda11__ctk_button_clicked (CtkButton* _sender, gpointer self);
static void _unity_panel_window_buttons_on_active_window_changed_bamf_matcher_active_window_changed (BamfMatcher* _sender, GObject* object, GObject* p0, gpointer self);
static void _lambda12_ (UnityPanelWindowButtons* self);
static gboolean _lambda13_ (UnityPanelWindowButtons* self);
static gboolean __lambda13__gsource_func (gpointer self);
static void __lambda12__unity_shell_active_window_state_changed (UnityShell* _sender, gpointer self);
static gboolean _lambda14_ (UnityPanelWindowButtons* self);
static gboolean __lambda14__gsource_func (gpointer self);
static GObject * unity_panel_window_buttons_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void unity_panel_window_buttons_finalize (GObject* obj);
#define UNITY_PANEL_WINDOW_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UNITY_PANEL_TYPE_WINDOW_BUTTON, UnityPanelWindowButtonPrivate))
enum  {
	UNITY_PANEL_WINDOW_BUTTON_DUMMY_PROPERTY,
	UNITY_PANEL_WINDOW_BUTTON_FILENAME
};
#define UNITY_PANEL_WINDOW_BUTTON_AMBIANCE "/usr/share/themes/Ambiance/metacity-1"
#define UNITY_PANEL_WINDOW_BUTTON_AMBIANCE_BETA "/usr/share/themes/Ambiance-maverick-beta/metacity-1"
static void unity_panel_window_button_real_get_preferred_width (ClutterActor* base, float for_height, float* min_width, float* nat_width);
static void unity_panel_window_button_real_get_preferred_height (ClutterActor* base, float for_width, float* min_height, float* nat_height);
const char* unity_panel_window_button_get_filename (UnityPanelWindowButton* self);
static void unity_panel_window_button_set_filename (UnityPanelWindowButton* self, const char* value);
static GObject * unity_panel_window_button_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void unity_panel_window_button_finalize (GObject* obj);
static void unity_panel_window_button_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void unity_panel_window_button_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);
static gint _vala_array_length (gpointer array);



UnityPanelWindowButtons* unity_panel_window_buttons_construct (GType object_type) {
	UnityPanelWindowButtons * self;
	self = (UnityPanelWindowButtons*) g_object_new (object_type, "orientation", CTK_ORIENTATION_HORIZONTAL, "spacing", 2, "homogeneous", FALSE, NULL);
	return self;
}


UnityPanelWindowButtons* unity_panel_window_buttons_new (void) {
	return unity_panel_window_buttons_construct (UNITY_PANEL_TYPE_WINDOW_BUTTONS);
}


static void _unity_panel_window_buttons_on_last_view_destroyed_gweak_notify (gpointer self, GObject* object) {
	unity_panel_window_buttons_on_last_view_destroyed (self, object);
}


static void unity_panel_window_buttons_on_active_window_changed (UnityPanelWindowButtons* self, GObject* object, GObject* object1) {
	GObject* _tmp0_;
	BamfView* new_view;
	g_return_if_fail (self != NULL);
	new_view = (_tmp0_ = object1, BAMF_IS_VIEW (_tmp0_) ? ((BamfView*) _tmp0_) : NULL);
	clutter_text_set_markup ((ClutterText*) self->priv->appname, "");
	clutter_actor_hide ((ClutterActor*) self->priv->appname);
	clutter_actor_hide ((ClutterActor*) self->priv->close);
	clutter_actor_hide ((ClutterActor*) self->priv->minimize);
	clutter_actor_hide ((ClutterActor*) self->priv->unmaximize);
	self->priv->last_xid = (guint32) 0;
	if (BAMF_IS_VIEW (object)) {
		if (object == G_OBJECT (self->priv->_last_view)) {
			g_object_weak_unref (object, _unity_panel_window_buttons_on_last_view_destroyed_gweak_notify, self);
		}
	}
	if (BAMF_IS_WINDOW (new_view)) {
		BamfView* _tmp1_;
		BamfWindow* win;
		gboolean allows_resize;
		gboolean is_maximized;
		BamfApplication* app;
		win = (_tmp1_ = new_view, BAMF_IS_WINDOW (_tmp1_) ? ((BamfWindow*) _tmp1_) : NULL);
		allows_resize = FALSE;
		is_maximized = FALSE;
		unity_shell_get_window_details (unity_global_shell, bamf_window_get_xid (win), &allows_resize, &is_maximized);
		if (is_maximized) {
			clutter_actor_hide ((ClutterActor*) self->priv->appname);
			clutter_actor_show ((ClutterActor*) self->priv->close);
			clutter_actor_show ((ClutterActor*) self->priv->minimize);
			clutter_actor_show ((ClutterActor*) self->priv->unmaximize);
		} else {
			clutter_actor_show ((ClutterActor*) self->priv->appname);
			clutter_actor_hide ((ClutterActor*) self->priv->close);
			clutter_actor_hide ((ClutterActor*) self->priv->minimize);
			clutter_actor_hide ((ClutterActor*) self->priv->unmaximize);
		}
		self->priv->last_xid = bamf_window_get_xid (win);
		app = bamf_matcher_get_active_application (self->priv->matcher);
		if (BAMF_IS_APPLICATION (app)) {
			GAppInfo* info;
			info = unity_app_info_manager_lookup (self->priv->appinfo, bamf_application_get_desktop_file (app));
			if (info != NULL) {
				char* display_name;
				char** _tmp2_;
				char** _tmp3_;
				gint _tmp3__length1;
				char* _tmp4_;
				char* _tmp5_;
				char* _tmp6_;
				display_name = g_strdup (g_app_info_get_display_name (info));
				display_name = (_tmp4_ = g_strdup ((_tmp3_ = _tmp2_ = g_strsplit (display_name, " ", 0), _tmp3__length1 = _vala_array_length (_tmp2_), _tmp3_)[0]), _g_free0 (display_name), _tmp4_);
				_tmp3_ = (_vala_array_free (_tmp3_, _tmp3__length1, (GDestroyNotify) g_free), NULL);
				clutter_text_set_markup ((ClutterText*) self->priv->appname, _tmp6_ = g_strdup_printf (UNITY_PANEL_WINDOW_BUTTONS_FORMAT, _tmp5_ = g_markup_escape_text (display_name, -1)));
				_g_free0 (_tmp6_);
				_g_free0 (_tmp5_);
				_g_free0 (display_name);
			} else {
				char* _tmp7_;
				char* _tmp8_;
				char* _tmp9_;
				clutter_text_set_markup ((ClutterText*) self->priv->appname, _tmp9_ = g_strdup_printf (UNITY_PANEL_WINDOW_BUTTONS_FORMAT, _tmp8_ = g_markup_escape_text (_tmp7_ = bamf_view_get_name ((BamfView*) win), -1)));
				_g_free0 (_tmp9_);
				_g_free0 (_tmp8_);
				_g_free0 (_tmp7_);
			}
			_g_object_unref0 (info);
		} else {
			char* _tmp10_;
			char* _tmp11_;
			char* _tmp12_;
			clutter_text_set_markup ((ClutterText*) self->priv->appname, _tmp12_ = g_strdup_printf (UNITY_PANEL_WINDOW_BUTTONS_FORMAT, _tmp11_ = g_markup_escape_text (_tmp10_ = bamf_view_get_name ((BamfView*) win), -1)));
			_g_free0 (_tmp12_);
			_g_free0 (_tmp11_);
			_g_free0 (_tmp10_);
		}
		self->priv->_last_view = new_view;
		g_object_weak_ref ((GObject*) self->priv->_last_view, _unity_panel_window_buttons_on_last_view_destroyed_gweak_notify, self);
	} else {
		self->priv->_last_view = NULL;
	}
}


static void unity_panel_window_buttons_on_last_view_destroyed (UnityPanelWindowButtons* self, GObject* object) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (object != NULL);
	if (object == G_OBJECT (self->priv->_last_view)) {
		self->priv->_last_view = NULL;
		clutter_text_set_markup ((ClutterText*) self->priv->appname, "");
		clutter_actor_hide ((ClutterActor*) self->priv->appname);
		clutter_actor_hide ((ClutterActor*) self->priv->close);
		clutter_actor_hide ((ClutterActor*) self->priv->minimize);
		clutter_actor_hide ((ClutterActor*) self->priv->unmaximize);
	}
}


static void unity_panel_window_buttons_real_get_preferred_width (ClutterActor* base, float for_height, float* min_width, float* nat_width) {
	UnityPanelWindowButtons * self;
	self = (UnityPanelWindowButtons*) base;
	*min_width = 72.0f;
	*nat_width = *min_width;
}


static void _lambda9_ (UnityPanelWindowButtons* self) {
	if (self->priv->last_xid > 0) {
		unity_shell_do_window_action (unity_global_shell, self->priv->last_xid, UNITY_WINDOW_ACTION_CLOSE);
	}
}


static void __lambda9__ctk_button_clicked (CtkButton* _sender, gpointer self) {
	_lambda9_ (self);
}


static void _lambda10_ (UnityPanelWindowButtons* self) {
	if (self->priv->last_xid > 0) {
		unity_shell_do_window_action (unity_global_shell, self->priv->last_xid, UNITY_WINDOW_ACTION_MINIMIZE);
	}
}


static void __lambda10__ctk_button_clicked (CtkButton* _sender, gpointer self) {
	_lambda10_ (self);
}


static void _lambda11_ (UnityPanelWindowButtons* self) {
	if (self->priv->last_xid > 0) {
		unity_shell_do_window_action (unity_global_shell, self->priv->last_xid, UNITY_WINDOW_ACTION_UNMAXIMIZE);
	}
}


static void __lambda11__ctk_button_clicked (CtkButton* _sender, gpointer self) {
	_lambda11_ (self);
}


static void _unity_panel_window_buttons_on_active_window_changed_bamf_matcher_active_window_changed (BamfMatcher* _sender, GObject* object, GObject* p0, gpointer self) {
	unity_panel_window_buttons_on_active_window_changed (self, object, p0);
}


static gboolean _lambda13_ (UnityPanelWindowButtons* self) {
	gboolean result = FALSE;
	BamfWindow* win;
	BamfWindow* _tmp0_;
	win = bamf_matcher_get_active_window (self->priv->matcher);
	unity_panel_window_buttons_on_active_window_changed (self, NULL, (_tmp0_ = win, G_IS_OBJECT (_tmp0_) ? ((GObject*) _tmp0_) : NULL));
	result = FALSE;
	return result;
}


static gboolean __lambda13__gsource_func (gpointer self) {
	gboolean result;
	result = _lambda13_ (self);
	return result;
}


static void _lambda12_ (UnityPanelWindowButtons* self) {
	g_timeout_add_full (G_PRIORITY_DEFAULT, (guint) 0, __lambda13__gsource_func, g_object_ref (self), g_object_unref);
}


static void __lambda12__unity_shell_active_window_state_changed (UnityShell* _sender, gpointer self) {
	_lambda12_ (self);
}


static gboolean _lambda14_ (UnityPanelWindowButtons* self) {
	gboolean result = FALSE;
	BamfWindow* win;
	BamfWindow* _tmp0_;
	win = bamf_matcher_get_active_window (self->priv->matcher);
	unity_panel_window_buttons_on_active_window_changed (self, NULL, (_tmp0_ = win, G_IS_OBJECT (_tmp0_) ? ((GObject*) _tmp0_) : NULL));
	result = FALSE;
	return result;
}


static gboolean __lambda14__gsource_func (gpointer self) {
	gboolean result;
	result = _lambda14_ (self);
	return result;
}


static GObject * unity_panel_window_buttons_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	UnityPanelWindowButtons * self;
	parent_class = G_OBJECT_CLASS (unity_panel_window_buttons_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = UNITY_PANEL_WINDOW_BUTTONS (obj);
	{
		CtkText* _tmp0_;
		UnityPanelWindowButton* _tmp1_;
		UnityPanelWindowButton* _tmp2_;
		UnityPanelWindowButton* _tmp3_;
		UnityAppInfoManager* _tmp4_;
		self->priv->appname = (_tmp0_ = g_object_ref_sink ((CtkText*) ctk_text_new (" ")), _g_object_unref0 (self->priv->appname), _tmp0_);
		clutter_text_set_use_markup ((ClutterText*) self->priv->appname, TRUE);
		clutter_text_set_max_length ((ClutterText*) self->priv->appname, 9);
		ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->appname, TRUE, TRUE);
		self->priv->close = (_tmp1_ = g_object_ref_sink (unity_panel_window_button_new ("close")), _g_object_unref0 (self->priv->close), _tmp1_);
		ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->close, FALSE, FALSE);
		g_signal_connect_object ((CtkButton*) self->priv->close, "clicked", (GCallback) __lambda9__ctk_button_clicked, self, 0);
		self->priv->minimize = (_tmp2_ = g_object_ref_sink (unity_panel_window_button_new ("minimize")), _g_object_unref0 (self->priv->minimize), _tmp2_);
		ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->minimize, FALSE, FALSE);
		g_signal_connect_object ((CtkButton*) self->priv->minimize, "clicked", (GCallback) __lambda10__ctk_button_clicked, self, 0);
		self->priv->unmaximize = (_tmp3_ = g_object_ref_sink (unity_panel_window_button_new ("unmaximize")), _g_object_unref0 (self->priv->unmaximize), _tmp3_);
		ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->unmaximize, FALSE, FALSE);
		g_signal_connect_object ((CtkButton*) self->priv->unmaximize, "clicked", (GCallback) __lambda11__ctk_button_clicked, self, 0);
		self->priv->appinfo = (_tmp4_ = unity_app_info_manager_get_instance (), _g_object_unref0 (self->priv->appinfo), _tmp4_);
		self->priv->matcher = bamf_matcher_get_default ();
		g_signal_connect_object (self->priv->matcher, "active-window-changed", (GCallback) _unity_panel_window_buttons_on_active_window_changed_bamf_matcher_active_window_changed, self, 0);
		g_signal_connect_object (unity_global_shell, "active-window-state-changed", (GCallback) __lambda12__unity_shell_active_window_state_changed, self, 0);
		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, __lambda14__gsource_func, g_object_ref (self), g_object_unref);
	}
	return obj;
}


static void unity_panel_window_buttons_class_init (UnityPanelWindowButtonsClass * klass) {
	unity_panel_window_buttons_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (UnityPanelWindowButtonsPrivate));
	CLUTTER_ACTOR_CLASS (klass)->get_preferred_width = unity_panel_window_buttons_real_get_preferred_width;
	G_OBJECT_CLASS (klass)->constructor = unity_panel_window_buttons_constructor;
	G_OBJECT_CLASS (klass)->finalize = unity_panel_window_buttons_finalize;
}


static void unity_panel_window_buttons_instance_init (UnityPanelWindowButtons * self) {
	self->priv = UNITY_PANEL_WINDOW_BUTTONS_GET_PRIVATE (self);
	self->priv->last_xid = (guint32) 0;
	self->priv->_last_view = NULL;
}


static void unity_panel_window_buttons_finalize (GObject* obj) {
	UnityPanelWindowButtons * self;
	self = UNITY_PANEL_WINDOW_BUTTONS (obj);
	_g_object_unref0 (self->priv->appname);
	_g_object_unref0 (self->priv->close);
	_g_object_unref0 (self->priv->minimize);
	_g_object_unref0 (self->priv->unmaximize);
	_g_object_unref0 (self->priv->appinfo);
	G_OBJECT_CLASS (unity_panel_window_buttons_parent_class)->finalize (obj);
}


GType unity_panel_window_buttons_get_type (void) {
	static volatile gsize unity_panel_window_buttons_type_id__volatile = 0;
	if (g_once_init_enter (&unity_panel_window_buttons_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnityPanelWindowButtonsClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_panel_window_buttons_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityPanelWindowButtons), 0, (GInstanceInitFunc) unity_panel_window_buttons_instance_init, NULL };
		GType unity_panel_window_buttons_type_id;
		unity_panel_window_buttons_type_id = g_type_register_static (CTK_TYPE_BOX, "UnityPanelWindowButtons", &g_define_type_info, 0);
		g_once_init_leave (&unity_panel_window_buttons_type_id__volatile, unity_panel_window_buttons_type_id);
	}
	return unity_panel_window_buttons_type_id__volatile;
}


UnityPanelWindowButton* unity_panel_window_button_construct (GType object_type, const char* filename) {
	UnityPanelWindowButton * self;
	g_return_val_if_fail (filename != NULL, NULL);
	self = (UnityPanelWindowButton*) g_object_new (object_type, "filename", filename, NULL);
	return self;
}


UnityPanelWindowButton* unity_panel_window_button_new (const char* filename) {
	return unity_panel_window_button_construct (UNITY_PANEL_TYPE_WINDOW_BUTTON, filename);
}


static void unity_panel_window_button_real_get_preferred_width (ClutterActor* base, float for_height, float* min_width, float* nat_width) {
	UnityPanelWindowButton * self;
	self = (UnityPanelWindowButton*) base;
	*min_width = (float) self->priv->icon_size;
	*nat_width = *min_width;
}


static void unity_panel_window_button_real_get_preferred_height (ClutterActor* base, float for_width, float* min_height, float* nat_height) {
	UnityPanelWindowButton * self;
	self = (UnityPanelWindowButton*) base;
	*min_height = (float) self->priv->icon_size;
	*nat_height = *min_height;
}


const char* unity_panel_window_button_get_filename (UnityPanelWindowButton* self) {
	const char* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_filename;
	return result;
}


static void unity_panel_window_button_set_filename (UnityPanelWindowButton* self, const char* value) {
	char* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->_filename = (_tmp0_ = g_strdup (value), _g_free0 (self->priv->_filename), _tmp0_);
	g_object_notify ((GObject *) self, "filename");
}


static const char* string_to_string (const char* self) {
	const char* result = NULL;
	g_return_val_if_fail (self != NULL, NULL);
	result = self;
	return result;
}


static GObject * unity_panel_window_button_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	UnityPanelWindowButton * self;
	GError * _inner_error_;
	parent_class = G_OBJECT_CLASS (unity_panel_window_button_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = UNITY_PANEL_WINDOW_BUTTON (obj);
	_inner_error_ = NULL;
	{
		if (self->priv->using_beta = g_file_test (UNITY_PANEL_WINDOW_BUTTON_AMBIANCE_BETA, G_FILE_TEST_EXISTS)) {
			char* _tmp0_;
			self->priv->icon_size = 19;
			self->priv->directory = (_tmp0_ = g_strdup (UNITY_PANEL_WINDOW_BUTTON_AMBIANCE_BETA), _g_free0 (self->priv->directory), _tmp0_);
		}
		{
			char* _tmp1_;
			char* _tmp2_;
			char* _tmp3_;
			ClutterActor* _tmp4_;
			char* _tmp5_;
			char* _tmp6_;
			char* _tmp7_;
			ClutterActor* _tmp8_;
			char* _tmp9_;
			char* _tmp10_;
			char* _tmp11_;
			ClutterActor* _tmp12_;
			self->bg = (_tmp4_ = (ClutterActor*) g_object_ref_sink ((CtkImage*) ctk_image_new_from_filename ((guint) self->priv->icon_size, _tmp3_ = g_strconcat (_tmp2_ = g_strconcat (_tmp1_ = g_strconcat (self->priv->directory, "/", NULL), self->priv->_filename, NULL), ".png", NULL))), _g_object_unref0 (self->bg), _tmp4_);
			_g_free0 (_tmp3_);
			_g_free0 (_tmp2_);
			_g_free0 (_tmp1_);
			ctk_actor_set_background_for_state ((CtkActor*) self, CTK_STATE_NORMAL, self->bg);
			clutter_actor_show (self->bg);
			self->bg = (_tmp8_ = (ClutterActor*) g_object_ref_sink ((CtkImage*) ctk_image_new_from_filename ((guint) self->priv->icon_size, _tmp7_ = g_strconcat (_tmp6_ = g_strconcat (_tmp5_ = g_strconcat (self->priv->directory, "/", NULL), self->priv->_filename, NULL), "_focused_prelight.png", NULL))), _g_object_unref0 (self->bg), _tmp8_);
			_g_free0 (_tmp7_);
			_g_free0 (_tmp6_);
			_g_free0 (_tmp5_);
			ctk_actor_set_background_for_state ((CtkActor*) self, CTK_STATE_PRELIGHT, self->bg);
			clutter_actor_show (self->bg);
			self->bg = (_tmp12_ = (ClutterActor*) g_object_ref_sink ((CtkImage*) ctk_image_new_from_filename ((guint) self->priv->icon_size, _tmp11_ = g_strconcat (_tmp10_ = g_strconcat (_tmp9_ = g_strconcat (self->priv->directory, "/", NULL), self->priv->_filename, NULL), "_focused_pressed.png", NULL))), _g_object_unref0 (self->bg), _tmp12_);
			_g_free0 (_tmp11_);
			_g_free0 (_tmp10_);
			_g_free0 (_tmp9_);
			ctk_actor_set_background_for_state ((CtkActor*) self, CTK_STATE_ACTIVE, self->bg);
			clutter_actor_show (self->bg);
		}
		goto __finally13;
		__catch13_g_error:
		{
			GError * e;
			e = _inner_error_;
			_inner_error_ = NULL;
			{
				char* _tmp13_;
				g_warning ("panel-window-buttons.vala:247: %s", _tmp13_ = g_strconcat ("Unable to load window button theme: You need Ambiance installed: ", string_to_string (e->message), NULL));
				_g_free0 (_tmp13_);
				_g_error_free0 (e);
			}
		}
		__finally13:
		if (_inner_error_ != NULL) {
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
			g_clear_error (&_inner_error_);
		}
	}
	return obj;
}


static void unity_panel_window_button_class_init (UnityPanelWindowButtonClass * klass) {
	unity_panel_window_button_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (UnityPanelWindowButtonPrivate));
	CLUTTER_ACTOR_CLASS (klass)->get_preferred_width = unity_panel_window_button_real_get_preferred_width;
	CLUTTER_ACTOR_CLASS (klass)->get_preferred_height = unity_panel_window_button_real_get_preferred_height;
	G_OBJECT_CLASS (klass)->get_property = unity_panel_window_button_get_property;
	G_OBJECT_CLASS (klass)->set_property = unity_panel_window_button_set_property;
	G_OBJECT_CLASS (klass)->constructor = unity_panel_window_button_constructor;
	G_OBJECT_CLASS (klass)->finalize = unity_panel_window_button_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), UNITY_PANEL_WINDOW_BUTTON_FILENAME, g_param_spec_string ("filename", "filename", "filename", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void unity_panel_window_button_instance_init (UnityPanelWindowButton * self) {
	self->priv = UNITY_PANEL_WINDOW_BUTTON_GET_PRIVATE (self);
	self->priv->using_beta = FALSE;
	self->priv->icon_size = 18;
	self->priv->directory = g_strdup (UNITY_PANEL_WINDOW_BUTTON_AMBIANCE);
}


static void unity_panel_window_button_finalize (GObject* obj) {
	UnityPanelWindowButton * self;
	self = UNITY_PANEL_WINDOW_BUTTON (obj);
	_g_free0 (self->priv->_filename);
	_g_object_unref0 (self->bg);
	_g_free0 (self->priv->directory);
	G_OBJECT_CLASS (unity_panel_window_button_parent_class)->finalize (obj);
}


GType unity_panel_window_button_get_type (void) {
	static volatile gsize unity_panel_window_button_type_id__volatile = 0;
	if (g_once_init_enter (&unity_panel_window_button_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnityPanelWindowButtonClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_panel_window_button_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityPanelWindowButton), 0, (GInstanceInitFunc) unity_panel_window_button_instance_init, NULL };
		GType unity_panel_window_button_type_id;
		unity_panel_window_button_type_id = g_type_register_static (CTK_TYPE_BUTTON, "UnityPanelWindowButton", &g_define_type_info, 0);
		g_once_init_leave (&unity_panel_window_button_type_id__volatile, unity_panel_window_button_type_id);
	}
	return unity_panel_window_button_type_id__volatile;
}


static void unity_panel_window_button_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	UnityPanelWindowButton * self;
	self = UNITY_PANEL_WINDOW_BUTTON (object);
	switch (property_id) {
		case UNITY_PANEL_WINDOW_BUTTON_FILENAME:
		g_value_set_string (value, unity_panel_window_button_get_filename (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void unity_panel_window_button_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	UnityPanelWindowButton * self;
	self = UNITY_PANEL_WINDOW_BUTTON (object);
	switch (property_id) {
		case UNITY_PANEL_WINDOW_BUTTON_FILENAME:
		unity_panel_window_button_set_filename (self, g_value_get_string (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	if ((array != NULL) && (destroy_func != NULL)) {
		int i;
		for (i = 0; i < array_length; i = i + 1) {
			if (((gpointer*) array)[i] != NULL) {
				destroy_func (((gpointer*) array)[i]);
			}
		}
	}
}


static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	_vala_array_destroy (array, array_length, destroy_func);
	g_free (array);
}


static gint _vala_array_length (gpointer array) {
	int length;
	length = 0;
	if (array) {
		while (((gpointer*) array)[length]) {
			length++;
		}
	}
	return length;
}




