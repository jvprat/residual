// Residual - Virtual machine to run LucasArts' 3D adventure games
// Copyright (C) 2003 The ScummVM-Residual Team (www.scummvm.org)
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

#ifndef MATERIAL_H
#define MATERIAL_H

#include "resource.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstring>

class Colormap;

class Material : public Resource {
public:
  // Load a texture from the given data.
  Material(const char *filename, const char *data, int len,
	   const Colormap &cmap);

  // Load this texture into the GL context
  void select() const;

  // Set which image in an animated texture to use
  void setNumber(int n) { curr_image_ = n; }

  int numImages() const { return num_images_; }
  int currentImage() const { return curr_image_; }

  ~Material();

private:
  int num_images_, curr_image_;
  int width_, height_;
  GLuint *textures_;
};

#endif