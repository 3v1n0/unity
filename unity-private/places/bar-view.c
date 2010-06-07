/* bar-view.c generated by valac, the Vala compiler
 * generated from bar-view.vala, do not modify */

/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <clutk/clutk.h>
#include <unity.h>
#include <gee.h>
#include <clutter/clutter.h>


#define UNITY_PLACES_BAR_TYPE_VIEW (unity_places_bar_view_get_type ())
#define UNITY_PLACES_BAR_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_BAR_TYPE_VIEW, UnityPlacesBarView))
#define UNITY_PLACES_BAR_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_BAR_TYPE_VIEW, UnityPlacesBarViewClass))
#define UNITY_PLACES_BAR_IS_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_BAR_TYPE_VIEW))
#define UNITY_PLACES_BAR_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_BAR_TYPE_VIEW))
#define UNITY_PLACES_BAR_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_BAR_TYPE_VIEW, UnityPlacesBarViewClass))

typedef struct _UnityPlacesBarView UnityPlacesBarView;
typedef struct _UnityPlacesBarViewClass UnityPlacesBarViewClass;
typedef struct _UnityPlacesBarViewPrivate UnityPlacesBarViewPrivate;

#define UNITY_PLACES_TYPE_MODEL (unity_places_model_get_type ())
#define UNITY_PLACES_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_TYPE_MODEL, UnityPlacesModel))
#define UNITY_PLACES_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_TYPE_MODEL, UnityPlacesModelClass))
#define UNITY_PLACES_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_TYPE_MODEL))
#define UNITY_PLACES_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_TYPE_MODEL))
#define UNITY_PLACES_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_TYPE_MODEL, UnityPlacesModelClass))

typedef struct _UnityPlacesModel UnityPlacesModel;
typedef struct _UnityPlacesModelClass UnityPlacesModelClass;

#define UNITY_PLACES_BAR_TYPE_PLACE_ICON (unity_places_bar_place_icon_get_type ())
#define UNITY_PLACES_BAR_PLACE_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_BAR_TYPE_PLACE_ICON, UnityPlacesBarPlaceIcon))
#define UNITY_PLACES_BAR_PLACE_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_BAR_TYPE_PLACE_ICON, UnityPlacesBarPlaceIconClass))
#define UNITY_PLACES_BAR_IS_PLACE_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_BAR_TYPE_PLACE_ICON))
#define UNITY_PLACES_BAR_IS_PLACE_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_BAR_TYPE_PLACE_ICON))
#define UNITY_PLACES_BAR_PLACE_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_BAR_TYPE_PLACE_ICON, UnityPlacesBarPlaceIconClass))

typedef struct _UnityPlacesBarPlaceIcon UnityPlacesBarPlaceIcon;
typedef struct _UnityPlacesBarPlaceIconClass UnityPlacesBarPlaceIconClass;

#define UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR (unity_places_cairo_drawing_places_vseparator_get_type ())
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_VSEPARATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR, UnityPlacesCairoDrawingPlacesVSeparator))
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_VSEPARATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR, UnityPlacesCairoDrawingPlacesVSeparatorClass))
#define UNITY_PLACES_CAIRO_DRAWING_IS_PLACES_VSEPARATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR))
#define UNITY_PLACES_CAIRO_DRAWING_IS_PLACES_VSEPARATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR))
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_VSEPARATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_VSEPARATOR, UnityPlacesCairoDrawingPlacesVSeparatorClass))

typedef struct _UnityPlacesCairoDrawingPlacesVSeparator UnityPlacesCairoDrawingPlacesVSeparator;
typedef struct _UnityPlacesCairoDrawingPlacesVSeparatorClass UnityPlacesCairoDrawingPlacesVSeparatorClass;

#define UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND (unity_places_cairo_drawing_places_background_get_type ())
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_BACKGROUND(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND, UnityPlacesCairoDrawingPlacesBackground))
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_BACKGROUND_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND, UnityPlacesCairoDrawingPlacesBackgroundClass))
#define UNITY_PLACES_CAIRO_DRAWING_IS_PLACES_BACKGROUND(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND))
#define UNITY_PLACES_CAIRO_DRAWING_IS_PLACES_BACKGROUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND))
#define UNITY_PLACES_CAIRO_DRAWING_PLACES_BACKGROUND_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_CAIRO_DRAWING_TYPE_PLACES_BACKGROUND, UnityPlacesCairoDrawingPlacesBackgroundClass))

typedef struct _UnityPlacesCairoDrawingPlacesBackground UnityPlacesCairoDrawingPlacesBackground;
typedef struct _UnityPlacesCairoDrawingPlacesBackgroundClass UnityPlacesCairoDrawingPlacesBackgroundClass;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define UNITY_PLACES_TYPE_PLACE (unity_places_place_get_type ())
#define UNITY_PLACES_PLACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_PLACES_TYPE_PLACE, UnityPlacesPlace))
#define UNITY_PLACES_PLACE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_PLACES_TYPE_PLACE, UnityPlacesPlaceClass))
#define UNITY_PLACES_IS_PLACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_PLACES_TYPE_PLACE))
#define UNITY_PLACES_IS_PLACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_PLACES_TYPE_PLACE))
#define UNITY_PLACES_PLACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_PLACES_TYPE_PLACE, UnityPlacesPlaceClass))

typedef struct _UnityPlacesPlace UnityPlacesPlace;
typedef struct _UnityPlacesPlaceClass UnityPlacesPlaceClass;
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))
typedef struct _UnityPlacesBarPlaceIconPrivate UnityPlacesBarPlaceIconPrivate;

struct _UnityPlacesBarView {
	CtkBox parent_instance;
	UnityPlacesBarViewPrivate * priv;
};

struct _UnityPlacesBarViewClass {
	CtkBoxClass parent_class;
};

struct _UnityPlacesBarViewPrivate {
	UnityPlacesModel* _model;
	UnityShell* _shell;
	GeeArrayList* places_icons;
	UnityPlacesBarPlaceIcon* trash_icon;
	UnityPlacesCairoDrawingPlacesVSeparator* separator;
	UnityPlacesCairoDrawingPlacesBackground* bg;
};

struct _UnityPlacesBarPlaceIcon {
	CtkImage parent_instance;
	UnityPlacesBarPlaceIconPrivate * priv;
};

struct _UnityPlacesBarPlaceIconClass {
	CtkImageClass parent_class;
};

struct _UnityPlacesBarPlaceIconPrivate {
	UnityPlacesPlace* _place;
};


static gpointer unity_places_bar_view_parent_class = NULL;
static gpointer unity_places_bar_place_icon_parent_class = NULL;

#define UNITY_PLACES_BAR_PANEL_HEIGHT ((float) 24)
#define UNITY_PLACES_BAR_ICON_SIZE 48
#define UNITY_PLACES_BAR_ICON_VIEW_WIDTH 80.0f
#define UNITY_PLACES_BAR_ICON_VIEW_Y1 8.0f
#define UNITY_PLACES_BAR_QL_PAD 12.0f
#define UNITY_PLACES_BAR_TRASH_FILE PKGDATADIR "/trash.png"
GType unity_places_bar_view_get_type (void);
GType unity_places_model_get_type (void);
GType unity_places_bar_place_icon_get_type (void);
GType unity_places_cairo_drawing_places_vseparator_get_type (void);
GType unity_places_cairo_drawing_places_background_get_type (void);
#define UNITY_PLACES_BAR_VIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UNITY_PLACES_BAR_TYPE_VIEW, UnityPlacesBarViewPrivate))
enum  {
	UNITY_PLACES_BAR_VIEW_DUMMY_PROPERTY,
	UNITY_PLACES_BAR_VIEW_MODEL,
	UNITY_PLACES_BAR_VIEW_SHELL
};
UnityPlacesCairoDrawingPlacesBackground* unity_places_cairo_drawing_places_background_new (void);
UnityPlacesCairoDrawingPlacesBackground* unity_places_cairo_drawing_places_background_construct (GType object_type);
UnityShell* unity_places_bar_view_get_shell (UnityPlacesBarView* self);
static void unity_places_bar_view_on_indicators_changed (UnityPlacesBarView* self, gint width);
static void _unity_places_bar_view_on_indicators_changed_unity_shell_indicators_changed (UnityShell* _sender, gint width, gpointer self);
UnityPlacesModel* unity_places_bar_view_get_model (UnityPlacesBarView* self);
GType unity_places_place_get_type (void);
static void unity_places_bar_view_on_place_added (UnityPlacesBarView* self, UnityPlacesPlace* place);
static void _unity_places_bar_view_on_place_added_unity_places_model_place_added (UnityPlacesModel* _sender, UnityPlacesPlace* place, gpointer self);
UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_new (gint width, const char* name, const char* icon_name, const char* tooltip);
UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_construct (GType object_type, gint width, const char* name, const char* icon_name, const char* tooltip);
gboolean unity_places_bar_view_on_button_release (UnityPlacesBarView* self, ClutterEvent* event);
static gboolean _unity_places_bar_view_on_button_release_clutter_actor_button_release_event (ClutterActor* _sender, ClutterEvent* event, gpointer self);
UnityPlacesCairoDrawingPlacesVSeparator* unity_places_cairo_drawing_places_vseparator_new (void);
UnityPlacesCairoDrawingPlacesVSeparator* unity_places_cairo_drawing_places_vseparator_construct (GType object_type);
UnityPlacesBarView* unity_places_bar_view_new (UnityPlacesModel* model, UnityShell* shell);
UnityPlacesBarView* unity_places_bar_view_construct (GType object_type, UnityPlacesModel* model, UnityShell* shell);
UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_new_from_place (gint size, UnityPlacesPlace* place);
UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_construct_from_place (GType object_type, gint size, UnityPlacesPlace* place);
void unity_places_cairo_drawing_places_background_create_places_background (UnityPlacesCairoDrawingPlacesBackground* self, gint WindowWidth, gint WindowHeight, gint TabPositionX, gint TabWidth, gint menu_width);
void unity_places_place_set_active (UnityPlacesPlace* self, gboolean value);
UnityPlacesPlace* unity_places_bar_place_icon_get_place (UnityPlacesBarPlaceIcon* self);
static void unity_places_bar_view_real_map (ClutterActor* base);
static void unity_places_bar_view_real_unmap (ClutterActor* base);
static void unity_places_bar_view_real_paint (ClutterActor* base);
static void unity_places_bar_view_real_allocate (ClutterActor* base, const ClutterActorBox* box, ClutterAllocationFlags flags);
static void unity_places_bar_view_set_model (UnityPlacesBarView* self, UnityPlacesModel* value);
static void unity_places_bar_view_set_shell (UnityPlacesBarView* self, UnityShell* value);
static void unity_places_bar_view_finalize (GObject* obj);
static void unity_places_bar_view_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void unity_places_bar_view_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
#define UNITY_PLACES_BAR_PLACE_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UNITY_PLACES_BAR_TYPE_PLACE_ICON, UnityPlacesBarPlaceIconPrivate))
enum  {
	UNITY_PLACES_BAR_PLACE_ICON_DUMMY_PROPERTY,
	UNITY_PLACES_BAR_PLACE_ICON_PLACE
};
const char* unity_places_place_get_name (UnityPlacesPlace* self);
const char* unity_places_place_get_icon_name (UnityPlacesPlace* self);
void unity_places_bar_place_icon_set_place (UnityPlacesBarPlaceIcon* self, UnityPlacesPlace* value);
static void unity_places_bar_place_icon_finalize (GObject* obj);
static void unity_places_bar_place_icon_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void unity_places_bar_place_icon_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);



static void _unity_places_bar_view_on_indicators_changed_unity_shell_indicators_changed (UnityShell* _sender, gint width, gpointer self) {
	unity_places_bar_view_on_indicators_changed (self, width);
}


static void _unity_places_bar_view_on_place_added_unity_places_model_place_added (UnityPlacesModel* _sender, UnityPlacesPlace* place, gpointer self) {
	unity_places_bar_view_on_place_added (self, place);
}


static gboolean _unity_places_bar_view_on_button_release_clutter_actor_button_release_event (ClutterActor* _sender, ClutterEvent* event, gpointer self) {
	gboolean result;
	result = unity_places_bar_view_on_button_release (self, event);
	return result;
}


UnityPlacesBarView* unity_places_bar_view_construct (GType object_type, UnityPlacesModel* model, UnityShell* shell) {
	UnityPlacesBarView * self;
	CtkEffectGlow* glow;
	ClutterColor _tmp0_ = {0};
	ClutterColor white;
	UnityPlacesCairoDrawingPlacesBackground* _tmp1_;
	GeeArrayList* _tmp2_;
	UnityPlacesBarPlaceIcon* _tmp3_;
	CtkEffectGlow* _tmp4_;
	UnityPlacesCairoDrawingPlacesVSeparator* _tmp5_;
	g_return_val_if_fail (model != NULL, NULL);
	g_return_val_if_fail (shell != NULL, NULL);
	glow = NULL;
	white = (_tmp0_.red = (guint8) 255, _tmp0_.green = (guint8) 255, _tmp0_.blue = (guint8) 255, _tmp0_.alpha = (guint8) 255, _tmp0_);
	self = (UnityPlacesBarView*) g_object_new (object_type, "model", model, "shell", shell, NULL);
	ctk_box_set_homogeneous ((CtkBox*) self, FALSE);
	ctk_box_set_orientation ((CtkBox*) self, (gint) CTK_ORIENTATION_HORIZONTAL);
	self->priv->bg = (_tmp1_ = g_object_ref_sink (unity_places_cairo_drawing_places_background_new ()), _g_object_unref0 (self->priv->bg), _tmp1_);
	clutter_actor_set_parent ((ClutterActor*) self->priv->bg, (ClutterActor*) self);
	clutter_actor_show ((ClutterActor*) self->priv->bg);
	g_signal_connect_object (self->priv->_shell, "indicators-changed", (GCallback) _unity_places_bar_view_on_indicators_changed_unity_shell_indicators_changed, self, 0);
	self->priv->places_icons = (_tmp2_ = gee_array_list_new (UNITY_PLACES_BAR_TYPE_PLACE_ICON, (GBoxedCopyFunc) g_object_ref, g_object_unref, NULL), _g_object_unref0 (self->priv->places_icons), _tmp2_);
	g_signal_connect_object (self->priv->_model, "place-added", (GCallback) _unity_places_bar_view_on_place_added_unity_places_model_place_added, self, 0);
	self->priv->trash_icon = (_tmp3_ = g_object_ref_sink (unity_places_bar_place_icon_new (UNITY_PLACES_BAR_ICON_SIZE, "Trash", UNITY_PLACES_BAR_TRASH_FILE, "Your piece of waste")), _g_object_unref0 (self->priv->trash_icon), _tmp3_);
	glow = (_tmp4_ = g_object_ref_sink ((CtkEffectGlow*) ctk_effect_glow_new ()), _g_object_unref0 (glow), _tmp4_);
	ctk_effect_glow_set_color (glow, &white);
	ctk_effect_glow_set_factor (glow, 1.0f);
	ctk_effect_set_margin ((CtkEffect*) glow, 6);
	ctk_actor_add_effect ((CtkActor*) self->priv->trash_icon, (CtkEffect*) glow);
	ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->trash_icon, FALSE, FALSE);
	g_signal_connect_object ((ClutterActor*) self->priv->trash_icon, "button-release-event", (GCallback) _unity_places_bar_view_on_button_release_clutter_actor_button_release_event, self, 0);
	self->priv->separator = (_tmp5_ = g_object_ref_sink (unity_places_cairo_drawing_places_vseparator_new ()), _g_object_unref0 (self->priv->separator), _tmp5_);
	ctk_box_pack ((CtkBox*) self, (ClutterActor*) self->priv->separator, FALSE, FALSE);
	clutter_actor_show_all ((ClutterActor*) self);
	_g_object_unref0 (glow);
	return self;
}


UnityPlacesBarView* unity_places_bar_view_new (UnityPlacesModel* model, UnityShell* shell) {
	return unity_places_bar_view_construct (UNITY_PLACES_BAR_TYPE_VIEW, model, shell);
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static void unity_places_bar_view_on_place_added (UnityPlacesBarView* self, UnityPlacesPlace* place) {
	ClutterColor _tmp0_ = {0};
	ClutterColor white;
	UnityPlacesBarPlaceIcon* icon;
	CtkEffectGlow* glow;
	g_return_if_fail (self != NULL);
	g_return_if_fail (place != NULL);
	white = (_tmp0_.red = (guint8) 255, _tmp0_.green = (guint8) 255, _tmp0_.blue = (guint8) 255, _tmp0_.alpha = (guint8) 255, _tmp0_);
	icon = g_object_ref_sink (unity_places_bar_place_icon_new_from_place (UNITY_PLACES_BAR_ICON_SIZE, place));
	gee_abstract_collection_add ((GeeAbstractCollection*) self->priv->places_icons, icon);
	glow = g_object_ref_sink ((CtkEffectGlow*) ctk_effect_glow_new ());
	ctk_effect_glow_set_color (glow, &white);
	ctk_effect_glow_set_factor (glow, 1.0f);
	ctk_effect_set_margin ((CtkEffect*) glow, 6);
	ctk_actor_add_effect ((CtkActor*) icon, (CtkEffect*) glow);
	ctk_box_pack ((CtkBox*) self, (ClutterActor*) icon, FALSE, FALSE);
	g_signal_connect_object ((ClutterActor*) icon, "button-release-event", (GCallback) _unity_places_bar_view_on_button_release_clutter_actor_button_release_event, self, 0);
	if (gee_collection_get_size ((GeeCollection*) self->priv->places_icons) == 1) {
		ClutterActor* stage;
		CtkPadding _tmp1_ = {0};
		stage = _g_object_ref0 (clutter_actor_get_stage ((ClutterActor*) icon));
		unity_places_cairo_drawing_places_background_create_places_background (self->priv->bg, (gint) clutter_actor_get_width (stage), (gint) clutter_actor_get_height (stage), (gint) ((ctk_actor_get_padding ((CtkActor*) self, &_tmp1_), _tmp1_.left) + UNITY_PLACES_BAR_QL_PAD), (gint) UNITY_PLACES_BAR_ICON_VIEW_WIDTH, unity_shell_get_indicators_width (self->priv->_shell));
		unity_places_place_set_active (unity_places_bar_place_icon_get_place (icon), TRUE);
		_g_object_unref0 (stage);
	}
	_g_object_unref0 (icon);
	_g_object_unref0 (glow);
}


static void unity_places_bar_view_on_indicators_changed (UnityPlacesBarView* self, gint width) {
	ClutterActor* stage;
	CtkPadding _tmp0_ = {0};
	g_return_if_fail (self != NULL);
	stage = _g_object_ref0 (clutter_actor_get_stage ((ClutterActor*) self));
	unity_places_cairo_drawing_places_background_create_places_background (self->priv->bg, (gint) clutter_actor_get_width (stage), (gint) clutter_actor_get_height (stage), (gint) ((ctk_actor_get_padding ((CtkActor*) self, &_tmp0_), _tmp0_.left) + UNITY_PLACES_BAR_QL_PAD), (gint) UNITY_PLACES_BAR_ICON_VIEW_WIDTH, width);
	clutter_actor_queue_relayout ((ClutterActor*) self);
	_g_object_unref0 (stage);
}


static void unity_places_bar_view_real_map (ClutterActor* base) {
	UnityPlacesBarView * self;
	self = (UnityPlacesBarView*) base;
	CLUTTER_ACTOR_CLASS (unity_places_bar_view_parent_class)->map ((ClutterActor*) CTK_BOX (self));
	clutter_actor_map ((ClutterActor*) self->priv->bg);
}


static void unity_places_bar_view_real_unmap (ClutterActor* base) {
	UnityPlacesBarView * self;
	self = (UnityPlacesBarView*) base;
	CLUTTER_ACTOR_CLASS (unity_places_bar_view_parent_class)->unmap ((ClutterActor*) CTK_BOX (self));
	clutter_actor_unmap ((ClutterActor*) self->priv->bg);
}


static void unity_places_bar_view_real_paint (ClutterActor* base) {
	UnityPlacesBarView * self;
	self = (UnityPlacesBarView*) base;
	clutter_actor_paint ((ClutterActor*) self->priv->bg);
	CLUTTER_ACTOR_CLASS (unity_places_bar_view_parent_class)->paint ((ClutterActor*) CTK_BOX (self));
}


static void unity_places_bar_view_real_allocate (ClutterActor* base, const ClutterActorBox* box, ClutterAllocationFlags flags) {
	UnityPlacesBarView * self;
	ClutterActorBox _tmp0_ = {0};
	ClutterActorBox child_box;
	gint n_places;
	CtkPadding _tmp1_ = {0};
	float lpadding;
	gint i_width;
	self = (UnityPlacesBarView*) base;
	child_box = (_tmp0_.x1 = (float) 0, _tmp0_.y1 = (float) 0, _tmp0_.x2 = (float) 0, _tmp0_.y2 = (float) 0, _tmp0_);
	child_box.x1 = 0.0f;
	child_box.x2 = (*box).x2 - (*box).x1;
	child_box.y1 = 0.0f;
	child_box.y2 = (*box).y2 - (*box).y1;
	clutter_actor_allocate ((ClutterActor*) self->priv->bg, &child_box, flags);
	n_places = 0;
	lpadding = (ctk_actor_get_padding ((CtkActor*) self, &_tmp1_), _tmp1_.left) + UNITY_PLACES_BAR_QL_PAD;
	child_box.y1 = UNITY_PLACES_BAR_ICON_VIEW_Y1;
	child_box.y2 = child_box.y1 + UNITY_PLACES_BAR_ICON_SIZE;
	{
		GeeIterator* _place_it;
		_place_it = gee_abstract_collection_iterator ((GeeAbstractCollection*) self->priv->places_icons);
		while (TRUE) {
			UnityPlacesBarPlaceIcon* place;
			if (!gee_iterator_next (_place_it)) {
				break;
			}
			place = (UnityPlacesBarPlaceIcon*) gee_iterator_get (_place_it);
			child_box.x1 = lpadding + (UNITY_PLACES_BAR_ICON_VIEW_WIDTH * n_places);
			child_box.x2 = child_box.x1 + UNITY_PLACES_BAR_ICON_VIEW_WIDTH;
			clutter_actor_allocate ((ClutterActor*) place, &child_box, flags);
			n_places++;
			_g_object_unref0 (place);
		}
		_g_object_unref0 (_place_it);
	}
	i_width = unity_shell_get_indicators_width (self->priv->_shell) + 8;
	child_box.x1 = (((*box).x2 - (*box).x1) - i_width) - UNITY_PLACES_BAR_ICON_VIEW_WIDTH;
	child_box.x2 = child_box.x1 + UNITY_PLACES_BAR_ICON_VIEW_WIDTH;
	clutter_actor_allocate ((ClutterActor*) self->priv->trash_icon, &child_box, flags);
	child_box.x1 = child_box.x1 - 12.0f;
	child_box.x2 = child_box.x1 + 5;
	child_box.y1 = (float) 10;
	child_box.y2 = (float) UNITY_PLACES_BAR_ICON_SIZE;
	clutter_actor_allocate ((ClutterActor*) self->priv->separator, &child_box, flags);
}


gboolean unity_places_bar_view_on_button_release (UnityPlacesBarView* self, ClutterEvent* event) {
	gboolean result = FALSE;
	GError * _inner_error_;
	ClutterActor* actor;
	ClutterActor* _tmp0_;
	g_return_val_if_fail (self != NULL, FALSE);
	_inner_error_ = NULL;
	actor = NULL;
	actor = (_tmp0_ = _g_object_ref0 ((*event).button.source), _g_object_unref0 (actor), _tmp0_);
	if (UNITY_PLACES_BAR_IS_PLACE_ICON (actor)) {
		ClutterActor* _tmp1_;
		UnityPlacesBarPlaceIcon* icon;
		icon = _g_object_ref0 ((_tmp1_ = actor, UNITY_PLACES_BAR_IS_PLACE_ICON (_tmp1_) ? ((UnityPlacesBarPlaceIcon*) _tmp1_) : NULL));
		if (actor == CLUTTER_ACTOR (self->priv->trash_icon)) {
			{
				g_spawn_command_line_async ("xdg-open trash:///", &_inner_error_);
				if (_inner_error_ != NULL) {
					if (_inner_error_->domain == G_SPAWN_ERROR) {
						goto __catch6_g_spawn_error;
					}
					_g_object_unref0 (icon);
					_g_object_unref0 (actor);
					g_critical ("file %s: line %d: unexpected error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
					g_clear_error (&_inner_error_);
					return FALSE;
				}
			}
			goto __finally6;
			__catch6_g_spawn_error:
			{
				GError * e;
				e = _inner_error_;
				_inner_error_ = NULL;
				{
					g_warning ("bar-view.vala:203: Unable to show Trash: %s", e->message);
					_g_error_free0 (e);
				}
			}
			__finally6:
			if (_inner_error_ != NULL) {
				_g_object_unref0 (icon);
				_g_object_unref0 (actor);
				g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
				g_clear_error (&_inner_error_);
				return FALSE;
			}
		} else {
			if (UNITY_PLACES_IS_PLACE (unity_places_bar_place_icon_get_place (icon))) {
				ClutterActor* stage;
				stage = _g_object_ref0 (clutter_actor_get_stage (actor));
				unity_places_cairo_drawing_places_background_create_places_background (self->priv->bg, (gint) clutter_actor_get_width (stage), (gint) clutter_actor_get_height (stage), (gint) clutter_actor_get_x (actor), 80, unity_shell_get_indicators_width (self->priv->_shell));
				{
					GeeIterator* _picon_it;
					_picon_it = gee_abstract_collection_iterator ((GeeAbstractCollection*) self->priv->places_icons);
					while (TRUE) {
						UnityPlacesBarPlaceIcon* picon;
						if (!gee_iterator_next (_picon_it)) {
							break;
						}
						picon = (UnityPlacesBarPlaceIcon*) gee_iterator_get (_picon_it);
						if (UNITY_PLACES_IS_PLACE (unity_places_bar_place_icon_get_place (picon))) {
							gboolean _tmp2_ = FALSE;
							if (picon == icon) {
								_tmp2_ = TRUE;
							} else {
								_tmp2_ = FALSE;
							}
							unity_places_place_set_active (unity_places_bar_place_icon_get_place (picon), _tmp2_);
						}
						_g_object_unref0 (picon);
					}
					_g_object_unref0 (_picon_it);
				}
				_g_object_unref0 (stage);
			}
		}
		result = TRUE;
		_g_object_unref0 (icon);
		_g_object_unref0 (actor);
		return result;
	}
	result = FALSE;
	_g_object_unref0 (actor);
	return result;
}


UnityPlacesModel* unity_places_bar_view_get_model (UnityPlacesBarView* self) {
	UnityPlacesModel* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_model;
	return result;
}


static void unity_places_bar_view_set_model (UnityPlacesBarView* self, UnityPlacesModel* value) {
	UnityPlacesModel* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->_model = (_tmp0_ = _g_object_ref0 (value), _g_object_unref0 (self->priv->_model), _tmp0_);
	g_object_notify ((GObject *) self, "model");
}


UnityShell* unity_places_bar_view_get_shell (UnityPlacesBarView* self) {
	UnityShell* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_shell;
	return result;
}


static void unity_places_bar_view_set_shell (UnityPlacesBarView* self, UnityShell* value) {
	UnityShell* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->_shell = (_tmp0_ = _g_object_ref0 (value), _g_object_unref0 (self->priv->_shell), _tmp0_);
	g_object_notify ((GObject *) self, "shell");
}


static void unity_places_bar_view_class_init (UnityPlacesBarViewClass * klass) {
	unity_places_bar_view_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (UnityPlacesBarViewPrivate));
	CLUTTER_ACTOR_CLASS (klass)->map = unity_places_bar_view_real_map;
	CLUTTER_ACTOR_CLASS (klass)->unmap = unity_places_bar_view_real_unmap;
	CLUTTER_ACTOR_CLASS (klass)->paint = unity_places_bar_view_real_paint;
	CLUTTER_ACTOR_CLASS (klass)->allocate = unity_places_bar_view_real_allocate;
	G_OBJECT_CLASS (klass)->get_property = unity_places_bar_view_get_property;
	G_OBJECT_CLASS (klass)->set_property = unity_places_bar_view_set_property;
	G_OBJECT_CLASS (klass)->finalize = unity_places_bar_view_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), UNITY_PLACES_BAR_VIEW_MODEL, g_param_spec_object ("model", "model", "model", UNITY_PLACES_TYPE_MODEL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass), UNITY_PLACES_BAR_VIEW_SHELL, g_param_spec_object ("shell", "shell", "shell", UNITY_TYPE_SHELL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void unity_places_bar_view_instance_init (UnityPlacesBarView * self) {
	self->priv = UNITY_PLACES_BAR_VIEW_GET_PRIVATE (self);
}


static void unity_places_bar_view_finalize (GObject* obj) {
	UnityPlacesBarView * self;
	self = UNITY_PLACES_BAR_VIEW (obj);
	_g_object_unref0 (self->priv->_model);
	_g_object_unref0 (self->priv->_shell);
	_g_object_unref0 (self->priv->places_icons);
	_g_object_unref0 (self->priv->trash_icon);
	_g_object_unref0 (self->priv->separator);
	_g_object_unref0 (self->priv->bg);
	G_OBJECT_CLASS (unity_places_bar_view_parent_class)->finalize (obj);
}


GType unity_places_bar_view_get_type (void) {
	static volatile gsize unity_places_bar_view_type_id__volatile = 0;
	if (g_once_init_enter (&unity_places_bar_view_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnityPlacesBarViewClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_places_bar_view_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityPlacesBarView), 0, (GInstanceInitFunc) unity_places_bar_view_instance_init, NULL };
		GType unity_places_bar_view_type_id;
		unity_places_bar_view_type_id = g_type_register_static (CTK_TYPE_BOX, "UnityPlacesBarView", &g_define_type_info, 0);
		g_once_init_leave (&unity_places_bar_view_type_id__volatile, unity_places_bar_view_type_id);
	}
	return unity_places_bar_view_type_id__volatile;
}


static void unity_places_bar_view_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	UnityPlacesBarView * self;
	self = UNITY_PLACES_BAR_VIEW (object);
	switch (property_id) {
		case UNITY_PLACES_BAR_VIEW_MODEL:
		g_value_set_object (value, unity_places_bar_view_get_model (self));
		break;
		case UNITY_PLACES_BAR_VIEW_SHELL:
		g_value_set_object (value, unity_places_bar_view_get_shell (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void unity_places_bar_view_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	UnityPlacesBarView * self;
	self = UNITY_PLACES_BAR_VIEW (object);
	switch (property_id) {
		case UNITY_PLACES_BAR_VIEW_MODEL:
		unity_places_bar_view_set_model (self, g_value_get_object (value));
		break;
		case UNITY_PLACES_BAR_VIEW_SHELL:
		unity_places_bar_view_set_shell (self, g_value_get_object (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_construct (GType object_type, gint width, const char* name, const char* icon_name, const char* tooltip) {
	UnityPlacesBarPlaceIcon * self;
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (icon_name != NULL, NULL);
	g_return_val_if_fail (tooltip != NULL, NULL);
	self = (UnityPlacesBarPlaceIcon*) g_object_new (object_type, "size", width, NULL);
	ctk_image_set_from_filename ((CtkImage*) self, icon_name);
	clutter_actor_set_reactive ((ClutterActor*) self, TRUE);
	return self;
}


UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_new (gint width, const char* name, const char* icon_name, const char* tooltip) {
	return unity_places_bar_place_icon_construct (UNITY_PLACES_BAR_TYPE_PLACE_ICON, width, name, icon_name, tooltip);
}


UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_construct_from_place (GType object_type, gint size, UnityPlacesPlace* place) {
	UnityPlacesBarPlaceIcon * self;
	g_return_val_if_fail (place != NULL, NULL);
	self = (UnityPlacesBarPlaceIcon*) unity_places_bar_place_icon_construct (object_type, size, unity_places_place_get_name (place), unity_places_place_get_icon_name (place), "");
	unity_places_bar_place_icon_set_place (self, place);
	return self;
}


UnityPlacesBarPlaceIcon* unity_places_bar_place_icon_new_from_place (gint size, UnityPlacesPlace* place) {
	return unity_places_bar_place_icon_construct_from_place (UNITY_PLACES_BAR_TYPE_PLACE_ICON, size, place);
}


UnityPlacesPlace* unity_places_bar_place_icon_get_place (UnityPlacesBarPlaceIcon* self) {
	UnityPlacesPlace* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_place;
	return result;
}


void unity_places_bar_place_icon_set_place (UnityPlacesBarPlaceIcon* self, UnityPlacesPlace* value) {
	UnityPlacesPlace* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->_place = (_tmp0_ = _g_object_ref0 (value), _g_object_unref0 (self->priv->_place), _tmp0_);
	g_object_notify ((GObject *) self, "place");
}


static void unity_places_bar_place_icon_class_init (UnityPlacesBarPlaceIconClass * klass) {
	unity_places_bar_place_icon_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (UnityPlacesBarPlaceIconPrivate));
	G_OBJECT_CLASS (klass)->get_property = unity_places_bar_place_icon_get_property;
	G_OBJECT_CLASS (klass)->set_property = unity_places_bar_place_icon_set_property;
	G_OBJECT_CLASS (klass)->finalize = unity_places_bar_place_icon_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), UNITY_PLACES_BAR_PLACE_ICON_PLACE, g_param_spec_object ("place", "place", "place", UNITY_PLACES_TYPE_PLACE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void unity_places_bar_place_icon_instance_init (UnityPlacesBarPlaceIcon * self) {
	self->priv = UNITY_PLACES_BAR_PLACE_ICON_GET_PRIVATE (self);
}


static void unity_places_bar_place_icon_finalize (GObject* obj) {
	UnityPlacesBarPlaceIcon * self;
	self = UNITY_PLACES_BAR_PLACE_ICON (obj);
	_g_object_unref0 (self->priv->_place);
	G_OBJECT_CLASS (unity_places_bar_place_icon_parent_class)->finalize (obj);
}


GType unity_places_bar_place_icon_get_type (void) {
	static volatile gsize unity_places_bar_place_icon_type_id__volatile = 0;
	if (g_once_init_enter (&unity_places_bar_place_icon_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnityPlacesBarPlaceIconClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_places_bar_place_icon_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityPlacesBarPlaceIcon), 0, (GInstanceInitFunc) unity_places_bar_place_icon_instance_init, NULL };
		GType unity_places_bar_place_icon_type_id;
		unity_places_bar_place_icon_type_id = g_type_register_static (CTK_TYPE_IMAGE, "UnityPlacesBarPlaceIcon", &g_define_type_info, 0);
		g_once_init_leave (&unity_places_bar_place_icon_type_id__volatile, unity_places_bar_place_icon_type_id);
	}
	return unity_places_bar_place_icon_type_id__volatile;
}


static void unity_places_bar_place_icon_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	UnityPlacesBarPlaceIcon * self;
	self = UNITY_PLACES_BAR_PLACE_ICON (object);
	switch (property_id) {
		case UNITY_PLACES_BAR_PLACE_ICON_PLACE:
		g_value_set_object (value, unity_places_bar_place_icon_get_place (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void unity_places_bar_place_icon_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	UnityPlacesBarPlaceIcon * self;
	self = UNITY_PLACES_BAR_PLACE_ICON (object);
	switch (property_id) {
		case UNITY_PLACES_BAR_PLACE_ICON_PLACE:
		unity_places_bar_place_icon_set_place (self, g_value_get_object (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}




