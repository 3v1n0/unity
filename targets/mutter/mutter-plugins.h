#ifndef MUTTER_PLUGINS
#define MUTTER_PLUGINS

#include <boxes.h>
#include <common.h>
#include <compositor.h>
#include <compositor-mutter.h>
#include <display.h>
#include <errors.h>
#include <gradient.h>
#include <errors.h>
#include <gradient.h>
#include <group.h>
#include <keybindings.h>
#include <main.h>
#include <mutter-plugin.h>
#include <mutter-window.h>
#include <prefs.h>
#include <preview-widget.h>
#include <screen.h>
#include <theme.h>
#include <theme-parser.h>
#include <types.h>
#include <util.h>
#include <window.h>
#include <workspace.h>

/* Stupid MUTTER_TYPE_WINDOW fix...who did the mutter GObject stuff? */
#define MUTTER_TYPE_WINDOW MUTTER_TYPE_COMP_WINDOW

#endif
