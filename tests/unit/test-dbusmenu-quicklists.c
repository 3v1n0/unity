/* test-dbusmenu-quicklists.c generated by valac, the Vala compiler
 * generated from test-dbusmenu-quicklists.vala, do not modify */

/*
 *      test-launcher.vala
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */

#include <glib.h>
#include <glib-object.h>
#include <unity-private.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem-proxy.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/server.h>
#include <clutk/clutk.h>
#include <string.h>
#include <gobject/gvaluecollector.h>


#define UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER (unity_tests_unit_test_scroller_child_controller_get_type ())
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER, UnityTestsUnitTestScrollerChildController))
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER, UnityTestsUnitTestScrollerChildControllerClass))
#define UNITY_TESTS_UNIT_IS_TEST_SCROLLER_CHILD_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER))
#define UNITY_TESTS_UNIT_IS_TEST_SCROLLER_CHILD_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER))
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER, UnityTestsUnitTestScrollerChildControllerClass))

typedef struct _UnityTestsUnitTestScrollerChildController UnityTestsUnitTestScrollerChildController;
typedef struct _UnityTestsUnitTestScrollerChildControllerClass UnityTestsUnitTestScrollerChildControllerClass;
typedef struct _UnityTestsUnitTestScrollerChildControllerPrivate UnityTestsUnitTestScrollerChildControllerPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE (unity_tests_unit_quicklist_suite_get_type ())
#define UNITY_TESTS_UNIT_QUICKLIST_SUITE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE, UnityTestsUnitQuicklistSuite))
#define UNITY_TESTS_UNIT_QUICKLIST_SUITE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE, UnityTestsUnitQuicklistSuiteClass))
#define UNITY_TESTS_UNIT_IS_QUICKLIST_SUITE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE))
#define UNITY_TESTS_UNIT_IS_QUICKLIST_SUITE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE))
#define UNITY_TESTS_UNIT_QUICKLIST_SUITE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE, UnityTestsUnitQuicklistSuiteClass))

typedef struct _UnityTestsUnitQuicklistSuite UnityTestsUnitQuicklistSuite;
typedef struct _UnityTestsUnitQuicklistSuiteClass UnityTestsUnitQuicklistSuiteClass;
typedef struct _UnityTestsUnitQuicklistSuitePrivate UnityTestsUnitQuicklistSuitePrivate;

#define UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD (unity_tests_unit_test_scroller_child_get_type ())
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD, UnityTestsUnitTestScrollerChild))
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD, UnityTestsUnitTestScrollerChildClass))
#define UNITY_TESTS_UNIT_IS_TEST_SCROLLER_CHILD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD))
#define UNITY_TESTS_UNIT_IS_TEST_SCROLLER_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD))
#define UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD, UnityTestsUnitTestScrollerChildClass))

typedef struct _UnityTestsUnitTestScrollerChild UnityTestsUnitTestScrollerChild;
typedef struct _UnityTestsUnitTestScrollerChildClass UnityTestsUnitTestScrollerChildClass;
typedef struct _UnityTestsUnitParamSpecQuicklistSuite UnityTestsUnitParamSpecQuicklistSuite;

struct _UnityTestsUnitTestScrollerChildController {
	UnityLauncherScrollerChildController parent_instance;
	UnityTestsUnitTestScrollerChildControllerPrivate * priv;
};

struct _UnityTestsUnitTestScrollerChildControllerClass {
	UnityLauncherScrollerChildControllerClass parent_class;
};

struct _UnityTestsUnitQuicklistSuite {
	GTypeInstance parent_instance;
	volatile int ref_count;
	UnityTestsUnitQuicklistSuitePrivate * priv;
};

struct _UnityTestsUnitQuicklistSuiteClass {
	GTypeClass parent_class;
	void (*finalize) (UnityTestsUnitQuicklistSuite *self);
};

struct _UnityTestsUnitParamSpecQuicklistSuite {
	GParamSpec parent_instance;
};


static gpointer unity_tests_unit_test_scroller_child_controller_parent_class = NULL;
static gpointer unity_tests_unit_quicklist_suite_parent_class = NULL;

GType unity_tests_unit_test_scroller_child_controller_get_type (void) G_GNUC_CONST;
enum  {
	UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CONTROLLER_DUMMY_PROPERTY
};
UnityTestsUnitTestScrollerChildController* unity_tests_unit_test_scroller_child_controller_new (UnityLauncherScrollerChild* child_);
UnityTestsUnitTestScrollerChildController* unity_tests_unit_test_scroller_child_controller_construct (GType object_type, UnityLauncherScrollerChild* child_);
static void unity_tests_unit_test_scroller_child_controller_real_get_menu_actions (UnityLauncherScrollerChildController* base, UnityLauncherScrollerChildControllermenu_cb callback, void* callback_target);
static void unity_tests_unit_test_scroller_child_controller_real_get_menu_navigation (UnityLauncherScrollerChildController* base, UnityLauncherScrollerChildControllermenu_cb callback, void* callback_target);
static void unity_tests_unit_test_scroller_child_controller_real_activate (UnityLauncherScrollerChildController* base);
static UnityLauncherQuicklistController* unity_tests_unit_test_scroller_child_controller_real_get_menu_controller (UnityLauncherScrollerChildController* base);
static GObject * unity_tests_unit_test_scroller_child_controller_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
gpointer unity_tests_unit_quicklist_suite_ref (gpointer instance);
void unity_tests_unit_quicklist_suite_unref (gpointer instance);
GParamSpec* unity_tests_unit_param_spec_quicklist_suite (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void unity_tests_unit_value_set_quicklist_suite (GValue* value, gpointer v_object);
void unity_tests_unit_value_take_quicklist_suite (GValue* value, gpointer v_object);
gpointer unity_tests_unit_value_get_quicklist_suite (const GValue* value);
GType unity_tests_unit_quicklist_suite_get_type (void) G_GNUC_CONST;
enum  {
	UNITY_TESTS_UNIT_QUICKLIST_SUITE_DUMMY_PROPERTY
};
static void unity_tests_unit_quicklist_suite_test_dbus_menu (UnityTestsUnitQuicklistSuite* self);
static void _unity_tests_unit_quicklist_suite_test_dbus_menu_gdata_test_func (gpointer self);
UnityTestsUnitQuicklistSuite* unity_tests_unit_quicklist_suite_new (void);
UnityTestsUnitQuicklistSuite* unity_tests_unit_quicklist_suite_construct (GType object_type);
UnityTestsUnitTestScrollerChild* unity_tests_unit_test_scroller_child_new (void);
UnityTestsUnitTestScrollerChild* unity_tests_unit_test_scroller_child_construct (GType object_type);
GType unity_tests_unit_test_scroller_child_get_type (void) G_GNUC_CONST;
static void unity_tests_unit_quicklist_suite_finalize (UnityTestsUnitQuicklistSuite* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



UnityTestsUnitTestScrollerChildController* unity_tests_unit_test_scroller_child_controller_construct (GType object_type, UnityLauncherScrollerChild* child_) {
	UnityTestsUnitTestScrollerChildController * self;
	g_return_val_if_fail (child_ != NULL, NULL);
	self = (UnityTestsUnitTestScrollerChildController*) g_object_new (object_type, "child", child_, NULL);
	return self;
}


UnityTestsUnitTestScrollerChildController* unity_tests_unit_test_scroller_child_controller_new (UnityLauncherScrollerChild* child_) {
	return unity_tests_unit_test_scroller_child_controller_construct (UNITY_TESTS_UNIT_TYPE_TEST_SCROLLER_CHILD_CONTROLLER, child_);
}


static void unity_tests_unit_test_scroller_child_controller_real_get_menu_actions (UnityLauncherScrollerChildController* base, UnityLauncherScrollerChildControllermenu_cb callback, void* callback_target) {
	UnityTestsUnitTestScrollerChildController * self;
	DbusmenuMenuitem* root;
	DbusmenuMenuitem* menuitem;
	DbusmenuMenuitem* _tmp0_;
	DbusmenuMenuitem* _tmp1_;
	DbusmenuMenuitem* _tmp2_;
	DbusmenuMenuitem* _tmp3_;
	DbusmenuMenuitem* _tmp4_;
	self = (UnityTestsUnitTestScrollerChildController*) base;
	root = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_set_root (root, TRUE);
	menuitem = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "Unchecked");
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_property_set_int (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp0_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp0_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "Checked");
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_property_set_int (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp1_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp1_);
	dbusmenu_menuitem_property_set (menuitem, "type", DBUSMENU_CLIENT_TYPES_SEPARATOR);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp2_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp2_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "A Label");
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp3_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp3_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "Radio Active");
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_property_set_int (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp4_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp4_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "Radio Unactive");
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_property_set_int (menuitem, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
	dbusmenu_menuitem_child_append (root, menuitem);
	callback (root, callback_target);
	_g_object_unref0 (menuitem);
	_g_object_unref0 (root);
}


static void unity_tests_unit_test_scroller_child_controller_real_get_menu_navigation (UnityLauncherScrollerChildController* base, UnityLauncherScrollerChildControllermenu_cb callback, void* callback_target) {
	UnityTestsUnitTestScrollerChildController * self;
	DbusmenuMenuitem* root;
	DbusmenuMenuitem* menuitem;
	DbusmenuMenuitem* _tmp0_;
	DbusmenuMenuitem* _tmp1_;
	self = (UnityTestsUnitTestScrollerChildController*) base;
	root = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_set_root (root, TRUE);
	menuitem = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "1");
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp0_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp0_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "2");
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_child_append (root, menuitem);
	menuitem = (_tmp1_ = dbusmenu_menuitem_new (), _g_object_unref0 (menuitem), _tmp1_);
	dbusmenu_menuitem_property_set (menuitem, DBUSMENU_MENUITEM_PROP_LABEL, "3");
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
	dbusmenu_menuitem_property_set_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
	dbusmenu_menuitem_child_append (root, menuitem);
	callback (root, callback_target);
	_g_object_unref0 (menuitem);
	_g_object_unref0 (root);
}


static void unity_tests_unit_test_scroller_child_controller_real_activate (UnityLauncherScrollerChildController* base) {
	UnityTestsUnitTestScrollerChildController * self;
	self = (UnityTestsUnitTestScrollerChildController*) base;
}


static UnityLauncherQuicklistController* unity_tests_unit_test_scroller_child_controller_real_get_menu_controller (UnityLauncherScrollerChildController* base) {
	UnityTestsUnitTestScrollerChildController * self;
	UnityLauncherQuicklistController* result = NULL;
	UnityLauncherQuicklistController* new_menu;
	self = (UnityTestsUnitTestScrollerChildController*) base;
	new_menu = (UnityLauncherQuicklistController*) unity_launcher_application_quicklist_controller_new ((UnityLauncherScrollerChildController*) self);
	result = new_menu;
	return result;
}


static GObject * unity_tests_unit_test_scroller_child_controller_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	UnityTestsUnitTestScrollerChildController * self;
	parent_class = G_OBJECT_CLASS (unity_tests_unit_test_scroller_child_controller_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = UNITY_TESTS_UNIT_TEST_SCROLLER_CHILD_CONTROLLER (obj);
	{
		unity_launcher_scroller_child_controller_set_name ((UnityLauncherScrollerChildController*) self, "Testing");
	}
	return obj;
}


static void unity_tests_unit_test_scroller_child_controller_class_init (UnityTestsUnitTestScrollerChildControllerClass * klass) {
	unity_tests_unit_test_scroller_child_controller_parent_class = g_type_class_peek_parent (klass);
	UNITY_LAUNCHER_SCROLLER_CHILD_CONTROLLER_CLASS (klass)->get_menu_actions = unity_tests_unit_test_scroller_child_controller_real_get_menu_actions;
	UNITY_LAUNCHER_SCROLLER_CHILD_CONTROLLER_CLASS (klass)->get_menu_navigation = unity_tests_unit_test_scroller_child_controller_real_get_menu_navigation;
	UNITY_LAUNCHER_SCROLLER_CHILD_CONTROLLER_CLASS (klass)->activate = unity_tests_unit_test_scroller_child_controller_real_activate;
	UNITY_LAUNCHER_SCROLLER_CHILD_CONTROLLER_CLASS (klass)->get_menu_controller = unity_tests_unit_test_scroller_child_controller_real_get_menu_controller;
	G_OBJECT_CLASS (klass)->constructor = unity_tests_unit_test_scroller_child_controller_constructor;
}


static void unity_tests_unit_test_scroller_child_controller_instance_init (UnityTestsUnitTestScrollerChildController * self) {
}


GType unity_tests_unit_test_scroller_child_controller_get_type (void) {
	static volatile gsize unity_tests_unit_test_scroller_child_controller_type_id__volatile = 0;
	if (g_once_init_enter (&unity_tests_unit_test_scroller_child_controller_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnityTestsUnitTestScrollerChildControllerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_tests_unit_test_scroller_child_controller_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityTestsUnitTestScrollerChildController), 0, (GInstanceInitFunc) unity_tests_unit_test_scroller_child_controller_instance_init, NULL };
		GType unity_tests_unit_test_scroller_child_controller_type_id;
		unity_tests_unit_test_scroller_child_controller_type_id = g_type_register_static (UNITY_LAUNCHER_TYPE_SCROLLER_CHILD_CONTROLLER, "UnityTestsUnitTestScrollerChildController", &g_define_type_info, 0);
		g_once_init_leave (&unity_tests_unit_test_scroller_child_controller_type_id__volatile, unity_tests_unit_test_scroller_child_controller_type_id);
	}
	return unity_tests_unit_test_scroller_child_controller_type_id__volatile;
}


static void _unity_tests_unit_quicklist_suite_test_dbus_menu_gdata_test_func (gpointer self) {
	unity_tests_unit_quicklist_suite_test_dbus_menu (self);
}


UnityTestsUnitQuicklistSuite* unity_tests_unit_quicklist_suite_construct (GType object_type) {
	UnityTestsUnitQuicklistSuite* self;
	self = (UnityTestsUnitQuicklistSuite*) g_type_create_instance (object_type);
	g_test_add_data_func ("/Unity/Launcher/TestDbusMenu", self, _unity_tests_unit_quicklist_suite_test_dbus_menu_gdata_test_func);
	return self;
}


UnityTestsUnitQuicklistSuite* unity_tests_unit_quicklist_suite_new (void) {
	return unity_tests_unit_quicklist_suite_construct (UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE);
}


static void unity_tests_unit_quicklist_suite_test_dbus_menu (UnityTestsUnitQuicklistSuite* self) {
	UnityLauncherScrollerChild* child;
	UnityTestsUnitTestScrollerChildController* controller;
	UnityLauncherQuicklistController* _tmp0_;
	UnityLauncherApplicationQuicklistController* menu;
	GList* menuitems;
	CtkMenuItem* _tmp1_;
	CtkMenuItem* _tmp2_;
	CtkMenuItem* _tmp3_;
	CtkMenuItem* _tmp4_;
	CtkMenuItem* _tmp5_;
	g_return_if_fail (self != NULL);
	child = (UnityLauncherScrollerChild*) g_object_ref_sink (unity_tests_unit_test_scroller_child_new ());
	controller = unity_tests_unit_test_scroller_child_controller_new (child);
	menu = (_tmp0_ = unity_launcher_scroller_child_controller_get_menu_controller ((UnityLauncherScrollerChildController*) controller), UNITY_LAUNCHER_IS_APPLICATION_QUICKLIST_CONTROLLER (_tmp0_) ? ((UnityLauncherApplicationQuicklistController*) _tmp0_) : NULL);
	unity_launcher_quicklist_controller_set_state ((UnityLauncherQuicklistController*) menu, UNITY_LAUNCHER_QUICKLIST_CONTROLLER_STATE_LABEL);
	menuitems = NULL;
	g_assert (g_list_length (menuitems) == 1);
	unity_launcher_quicklist_controller_set_state ((UnityLauncherQuicklistController*) menu, UNITY_LAUNCHER_QUICKLIST_CONTROLLER_STATE_MENU);
	g_assert (g_list_length (menuitems) >= 10);
	g_assert (CTK_IS_CHECK_MENU_ITEM ((CtkMenuItem*) menuitems->data));
	g_assert (ctk_check_menu_item_get_active ((_tmp1_ = (CtkMenuItem*) menuitems->data, CTK_IS_CHECK_MENU_ITEM (_tmp1_) ? ((CtkCheckMenuItem*) _tmp1_) : NULL)) == FALSE);
	menuitems = menuitems->next;
	g_assert (CTK_IS_CHECK_MENU_ITEM ((CtkMenuItem*) menuitems->data));
	g_assert (ctk_check_menu_item_get_active ((_tmp2_ = (CtkMenuItem*) menuitems->data, CTK_IS_CHECK_MENU_ITEM (_tmp2_) ? ((CtkCheckMenuItem*) _tmp2_) : NULL)) == TRUE);
	menuitems = menuitems->next;
	g_assert (CTK_IS_MENU_SEPERATOR ((CtkMenuItem*) menuitems->data));
	menuitems = menuitems->next;
	g_assert (CTK_IS_MENU_ITEM ((CtkMenuItem*) menuitems->data));
	g_assert (_vala_strcmp0 (ctk_menu_item_get_label ((_tmp3_ = (CtkMenuItem*) menuitems->data, CTK_IS_MENU_ITEM (_tmp3_) ? ((CtkMenuItem*) _tmp3_) : NULL)), "A Label") == 0);
	menuitems = menuitems->next;
	g_assert (CTK_IS_RADIO_MENU_ITEM ((CtkMenuItem*) menuitems->data));
	g_assert (ctk_check_menu_item_get_active ((CtkCheckMenuItem*) (_tmp4_ = (CtkMenuItem*) menuitems->data, CTK_IS_RADIO_MENU_ITEM (_tmp4_) ? ((CtkRadioMenuItem*) _tmp4_) : NULL)) == TRUE);
	menuitems = menuitems->next;
	g_assert (CTK_IS_RADIO_MENU_ITEM ((CtkMenuItem*) menuitems->data));
	g_assert (ctk_check_menu_item_get_active ((CtkCheckMenuItem*) (_tmp5_ = (CtkMenuItem*) menuitems->data, CTK_IS_RADIO_MENU_ITEM (_tmp5_) ? ((CtkRadioMenuItem*) _tmp5_) : NULL)) == TRUE);
	menuitems = menuitems->next;
	_g_object_unref0 (menu);
	_g_object_unref0 (controller);
	_g_object_unref0 (child);
}


static void unity_tests_unit_value_quicklist_suite_init (GValue* value) {
	value->data[0].v_pointer = NULL;
}


static void unity_tests_unit_value_quicklist_suite_free_value (GValue* value) {
	if (value->data[0].v_pointer) {
		unity_tests_unit_quicklist_suite_unref (value->data[0].v_pointer);
	}
}


static void unity_tests_unit_value_quicklist_suite_copy_value (const GValue* src_value, GValue* dest_value) {
	if (src_value->data[0].v_pointer) {
		dest_value->data[0].v_pointer = unity_tests_unit_quicklist_suite_ref (src_value->data[0].v_pointer);
	} else {
		dest_value->data[0].v_pointer = NULL;
	}
}


static gpointer unity_tests_unit_value_quicklist_suite_peek_pointer (const GValue* value) {
	return value->data[0].v_pointer;
}


static gchar* unity_tests_unit_value_quicklist_suite_collect_value (GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
	if (collect_values[0].v_pointer) {
		UnityTestsUnitQuicklistSuite* object;
		object = collect_values[0].v_pointer;
		if (object->parent_instance.g_class == NULL) {
			return g_strconcat ("invalid unclassed object pointer for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		} else if (!g_value_type_compatible (G_TYPE_FROM_INSTANCE (object), G_VALUE_TYPE (value))) {
			return g_strconcat ("invalid object type `", g_type_name (G_TYPE_FROM_INSTANCE (object)), "' for value type `", G_VALUE_TYPE_NAME (value), "'", NULL);
		}
		value->data[0].v_pointer = unity_tests_unit_quicklist_suite_ref (object);
	} else {
		value->data[0].v_pointer = NULL;
	}
	return NULL;
}


static gchar* unity_tests_unit_value_quicklist_suite_lcopy_value (const GValue* value, guint n_collect_values, GTypeCValue* collect_values, guint collect_flags) {
	UnityTestsUnitQuicklistSuite** object_p;
	object_p = collect_values[0].v_pointer;
	if (!object_p) {
		return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
	}
	if (!value->data[0].v_pointer) {
		*object_p = NULL;
	} else if (collect_flags && G_VALUE_NOCOPY_CONTENTS) {
		*object_p = value->data[0].v_pointer;
	} else {
		*object_p = unity_tests_unit_quicklist_suite_ref (value->data[0].v_pointer);
	}
	return NULL;
}


GParamSpec* unity_tests_unit_param_spec_quicklist_suite (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags) {
	UnityTestsUnitParamSpecQuicklistSuite* spec;
	g_return_val_if_fail (g_type_is_a (object_type, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE), NULL);
	spec = g_param_spec_internal (G_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
	G_PARAM_SPEC (spec)->value_type = object_type;
	return G_PARAM_SPEC (spec);
}


gpointer unity_tests_unit_value_get_quicklist_suite (const GValue* value) {
	g_return_val_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE), NULL);
	return value->data[0].v_pointer;
}


void unity_tests_unit_value_set_quicklist_suite (GValue* value, gpointer v_object) {
	UnityTestsUnitQuicklistSuite* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
		unity_tests_unit_quicklist_suite_ref (value->data[0].v_pointer);
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		unity_tests_unit_quicklist_suite_unref (old);
	}
}


void unity_tests_unit_value_take_quicklist_suite (GValue* value, gpointer v_object) {
	UnityTestsUnitQuicklistSuite* old;
	g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (value, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE));
	old = value->data[0].v_pointer;
	if (v_object) {
		g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (v_object, UNITY_TESTS_UNIT_TYPE_QUICKLIST_SUITE));
		g_return_if_fail (g_value_type_compatible (G_TYPE_FROM_INSTANCE (v_object), G_VALUE_TYPE (value)));
		value->data[0].v_pointer = v_object;
	} else {
		value->data[0].v_pointer = NULL;
	}
	if (old) {
		unity_tests_unit_quicklist_suite_unref (old);
	}
}


static void unity_tests_unit_quicklist_suite_class_init (UnityTestsUnitQuicklistSuiteClass * klass) {
	unity_tests_unit_quicklist_suite_parent_class = g_type_class_peek_parent (klass);
	UNITY_TESTS_UNIT_QUICKLIST_SUITE_CLASS (klass)->finalize = unity_tests_unit_quicklist_suite_finalize;
}


static void unity_tests_unit_quicklist_suite_instance_init (UnityTestsUnitQuicklistSuite * self) {
	self->ref_count = 1;
}


static void unity_tests_unit_quicklist_suite_finalize (UnityTestsUnitQuicklistSuite* obj) {
	UnityTestsUnitQuicklistSuite * self;
	self = UNITY_TESTS_UNIT_QUICKLIST_SUITE (obj);
}


GType unity_tests_unit_quicklist_suite_get_type (void) {
	static volatile gsize unity_tests_unit_quicklist_suite_type_id__volatile = 0;
	if (g_once_init_enter (&unity_tests_unit_quicklist_suite_type_id__volatile)) {
		static const GTypeValueTable g_define_type_value_table = { unity_tests_unit_value_quicklist_suite_init, unity_tests_unit_value_quicklist_suite_free_value, unity_tests_unit_value_quicklist_suite_copy_value, unity_tests_unit_value_quicklist_suite_peek_pointer, "p", unity_tests_unit_value_quicklist_suite_collect_value, "p", unity_tests_unit_value_quicklist_suite_lcopy_value };
		static const GTypeInfo g_define_type_info = { sizeof (UnityTestsUnitQuicklistSuiteClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_tests_unit_quicklist_suite_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnityTestsUnitQuicklistSuite), 0, (GInstanceInitFunc) unity_tests_unit_quicklist_suite_instance_init, &g_define_type_value_table };
		static const GTypeFundamentalInfo g_define_type_fundamental_info = { (G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE) };
		GType unity_tests_unit_quicklist_suite_type_id;
		unity_tests_unit_quicklist_suite_type_id = g_type_register_fundamental (g_type_fundamental_next (), "UnityTestsUnitQuicklistSuite", &g_define_type_info, &g_define_type_fundamental_info, 0);
		g_once_init_leave (&unity_tests_unit_quicklist_suite_type_id__volatile, unity_tests_unit_quicklist_suite_type_id);
	}
	return unity_tests_unit_quicklist_suite_type_id__volatile;
}


gpointer unity_tests_unit_quicklist_suite_ref (gpointer instance) {
	UnityTestsUnitQuicklistSuite* self;
	self = instance;
	g_atomic_int_inc (&self->ref_count);
	return instance;
}


void unity_tests_unit_quicklist_suite_unref (gpointer instance) {
	UnityTestsUnitQuicklistSuite* self;
	self = instance;
	if (g_atomic_int_dec_and_test (&self->ref_count)) {
		UNITY_TESTS_UNIT_QUICKLIST_SUITE_GET_CLASS (self)->finalize (self);
		g_type_free_instance ((GTypeInstance *) self);
	}
}


static int _vala_strcmp0 (const char * str1, const char * str2) {
	if (str1 == NULL) {
		return -(str1 != str2);
	}
	if (str2 == NULL) {
		return str1 != str2;
	}
	return strcmp (str1, str2);
}




