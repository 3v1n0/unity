/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#include <X11/extensions/XTest.h>
#include <X11/keysym.h> 

#ifndef NUX_AUTOMATED_TEST_FRAMEWORK_H
#define NUX_AUTOMATED_TEST_FRAMEWORK_H

class NuxAutomatedTestFramework
{
public:
  NuxAutomatedTestFramework(nux::WindowThread *window_thread);
  virtual ~NuxAutomatedTestFramework();

  //! Initialize the testing framework.
  void Startup();

  //! Simulate a mouse click event on a view.
  /*!
      Move the mouse to the middle of the view (if it isn't there already) and perform a click event.
  */  
  void ViewSendMouseClick(nux::View *view, int button);
  //! Simulate a mouse double click event on a view.
  /*!
      Move the mouse to the middle of the view (if it isn't there already) and perform a double click event.
  */  
  void ViewSendMouseDoubleClick(nux::View *view, int button);
  //! Simulate a mouse down event on a view.
  void ViewSendMouseDown(nux::View *view, int button);
  //! Simulate a mouse up event on a view.
  void ViewSendMouseUp(nux::View *view, int button);
  //! Simulate a drag event on a view from (x0, y0) to (x1, y1).
  void ViewSendMouseDrag(nux::View *view, int button, int x0, int y0, int x1, int y1);
  //! Simulate mouse motion to (x, y).
  void ViewSendMouseMotionTo(nux::View *view, int x, int y);
  //! Simulate mouse motion to the center of a view.
  void ViewSendMouseMotionToCenter(nux::View *view);
  //! Simulate mouse motion to the top left corner of a view.
  void ViewSendMouseMotionToTopLeft(nux::View *view);
  //! Simulate mouse motion to the top right corner of a view.
  void ViewSendMouseMotionToTopRight(nux::View *view);
  //! Simulate mouse motion to the bottom left corner of a view.
  void ViewSendMouseMotionToBottomLeft(nux::View *view);
  //! Simulate mouse motion to the bottom right corner of a view.
  void ViewSendMouseMotionToBottomRight(nux::View *view);

  //! Simulate a key event.
  void ViewSendChar(const char c);
  //! Simulate a succession of key events.
  void ViewSendString(const std::string &str);
  //! Simulate a key combo.
  void ViewSendKeyCombo(KeySym modsym0, KeySym modsym1, KeySym modsym2, const char c);
  //! Simulate Ctrl+a.
  void ViewSendCtrlA();
  //! Simulate Delete key.
  void ViewSendDelete();
  //! Simulate Backspace key.
  void ViewSendBackspace();
  //! Simulate Escape key.
  void ViewSendEscape();
  //! Simulate Tab key.
  void ViewSendTab();
  //! Simulate Return key.
  void ViewSendReturn();

  //! Put the mouse pointer anywhere on the display.
  void PutMouseAt(int x, int y);

  //! Simulate a mouse event.
  void SendFakeMouseEvent(int mouse_button_index, bool pressed);
  //! Simulate a key event.
  void SendFakeKeyEvent(KeySym keysym, KeySym modsym);
  //! Simulate a mouse motion event.
  void SendFakeMouseMotionEvent(int x, int y, int ms_delay);

  /*!
      Set the test thread to terminae the program when testing is over.
  */
  void SetTerminateProgramWhenDone(bool terminate);
  /*!
      Return true if the test thread is allowed to terminate the program after testing is over.
  */
  bool WhenDoneTerminateProgram();

  /*!
      Print a report message to the console.
  */
  void TestReportMsg(bool b, const char* msg);

private:
  void WindowConfigSignal(int x, int y, int width, int height);
  
  bool ready_to_start_;
  Display* display_;
  nux::WindowThread *window_thread_;
  int window_x_;
  int window_y_;
  int window_width_;
  int window_height_;
  bool terminate_when_test_over_;

  static int mouse_motion_time_span;    // in milliseconds
  static int mouse_click_time_span;     // in milliseconds
  static int minimum_sleep_time;        // in milliseconds   
  static int safety_border_inside_view; // in pixels
};

#endif // NUX_AUTOMATED_TEST_FRAMEWORK_H

