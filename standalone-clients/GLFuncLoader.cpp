// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2010-11 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "GLFuncLoader.h"
#include <dlfcn.h>

unity::GLLoader::FuncPtr unity::GLLoader::getProcAddr(const std::string &name)
{
  static void *dlhand = NULL;
  FuncPtr funcPtr = NULL;

  glGetError ();

  if (!funcPtr)
  {
    if (!dlhand)
      dlhand = dlopen ("libglfuncloader.so", RTLD_LAZY);

    char *error =  dlerror ();

    if (dlhand)
    {
      dlerror ();
      funcPtr = (FuncPtr) dlsym (dlhand, name.c_str ());

      error = dlerror ();
      if (error != NULL)
        funcPtr = NULL;
    }
  }

  return funcPtr;
}


