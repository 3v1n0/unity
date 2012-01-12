==============
Manual Testing
==============

Outline
-------
Sometimes it is not possible to easily make an automated test for some
use-cases.  However just because something isn't easily tested automatically
doesn't mean that there isn't value in creating a test for it, just that the
test needs to be executed by a person until it is converted into an automated
test.


Format
------
Manual tests take the format of text files in this directory (the manual-test
one).  These files are formatted using `reStructured Text`_.  A very
comprehensive `quick reference`_ is available.

.. _reStructured Text: http://en.wikipedia.org/wiki/ReStructuredText
.. _quick reference: http://docutils.sourceforge.net/docs/user/rst/quickref.html

Tests have a header, steps to follow, and an expected outcome, as demonstrated
by the following example:

There can be multiple tests in a single file, but they should all be related,
and the filename should indicate the tests it contains.


Test Dash
---------
This test shows that the dash appears when the super key is pushed.

#. Start with a clear screen
#. Press the <super> key

Outcome
  The dash appears, and focus is in the search box.  The icons on the laucher
  are desaturated except for the ubuntu button at the top.  The icons in the
  panel go white.


Directory Structure
-------------------
It is expected that as we grow a number of manual tests, we will use
directories to organise them.

