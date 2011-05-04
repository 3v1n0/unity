/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */


#include <glib.h>
#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <core/atoms.h>

#include "unitymtgrabhandles_options.h"

namespace Unity
{
    namespace MT
    {
        typedef std::pair <GLTexture::List, CompSize> TextureSize;
	typedef std::pair <GLTexture::List *, CompRect *> TextureLayout;


	class GrabHandle :
            public CompRect
	{
	    public:

		GrabHandle (TextureSize *texture, Window owner, unsigned int id);
		~GrabHandle ();

		void handleButtonPress (XButtonEvent *ev);
		void reposition (CompPoint *p, bool);

		void show ();
		void hide ();

		TextureLayout layout ();

	    private:

		Window		mIpw;
		Window		mOwner;
		TextureSize     *mTexture;
		unsigned int    mId;
	};

	class GrabHandleGroup :
	    public std::vector <Unity::MT::GrabHandle>
	{
	    public:

		GrabHandleGroup (Window owner);
		~GrabHandleGroup ();

		void relayout (const CompRect &, bool);
		void restack ();

		bool visible ();
		bool animate (unsigned int);
		bool needsAnimate ();

		int opacity ();

                void hide ();
                void show ();

		std::vector <TextureLayout> layout ();

	    private:

		typedef enum _state {
		    FADE_IN = 1,
		    FADE_OUT,
		    NONE
		} State;

		State  mState;
		int    mOpacity;

		bool mMoreAnimate;
	};
    };
};

class UnityMTGrabHandlesScreen :
    public PluginClassHandler <UnityMTGrabHandlesScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public UnitymtgrabhandlesOptions
{
    public:

	UnityMTGrabHandlesScreen (CompScreen *);
	~UnityMTGrabHandlesScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

    public:

	void handleEvent (XEvent *);

	bool toggleHandles (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options);

  	bool showHandles (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options);  

	bool hideHandles (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options);  

	void addHandles (Unity::MT::GrabHandleGroup *handles);
	void removeHandles (Unity::MT::GrabHandleGroup *handles);

	void addHandleWindow (Unity::MT::GrabHandle *, Window);
	void removeHandleWindow (Window);

	void preparePaint (int);
        void donePaint ();

	std::vector <Unity::MT::TextureSize>  & textures () { return mHandleTextures; }

    private:

	std::list <Unity::MT::GrabHandleGroup *> mGrabHandles;
	std::vector <Unity::MT::TextureSize> mHandleTextures;

	std::map <Window, Unity::MT::GrabHandle *> mInputHandles;
	CompWindowVector			   mLastClientListStacking;
	Atom					   mCompResizeWindowAtom;

	bool					mMoreAnimate;
	
	// hack
	friend class UnityMTGrabHandlesWindow;
};

#define UMTGH_SCREEN(screen)						      \
    UnityMTGrabHandlesScreen *us = UnityMTGrabHandlesScreen::get (screen)

class UnityMTGrabHandlesWindow :
    public PluginClassHandler <UnityMTGrabHandlesWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	UnityMTGrabHandlesWindow (CompWindow *);
	~UnityMTGrabHandlesWindow ();

	CompWindow	*window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

    public:

	bool allowHandles ();
	bool handleTimerActive ();

	void grabNotify (int, int, unsigned int, unsigned int);
	void ungrabNotify ();

	void relayout (const CompRect &, bool);

	void getOutputExtents (CompWindowExtents &output);

	void moveNotify (int dx, int dy, bool immediate);

	bool glDraw (const GLMatrix &,
		     GLFragment::Attrib &,
		     const CompRegion &,
		     unsigned int);

 	bool    handlesVisible ();
	void    hideHandles ();
	void    showHandles (bool use_timer);
	void    restackHandles ();

	void resetTimer ();
	void disableTimer ();

    private:
  
	static gboolean onHideTimeout (gpointer data);

	Unity::MT::GrabHandleGroup *mHandles;
	UnityMTGrabHandlesScreen *_mt_screen;
	guint _timer_handle;
	
	friend class Unity::MT::GrabHandle;
};

#define UMTGH_WINDOW(window)						      \
    UnityMTGrabHandlesWindow *uw = UnityMTGrabHandlesWindow::get (window)

class UnityMTGrabHandlesPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <UnityMTGrabHandlesScreen,
						 UnityMTGrabHandlesWindow>
{
    public:

	bool init ();
};
