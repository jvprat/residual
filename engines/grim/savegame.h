/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * $URL$
 * $Id$
 *
 */

#ifndef GRIM_SAVEGAME_H
#define GRIM_SAVEGAME_H

#include "common/savefile.h"

namespace Grim {

class SaveGame {
public:
	SaveGame(const char *filename, bool saving);
	~SaveGame();

	uint32 beginSection(uint32 sectionTag);
	void endSection();
	uint32 getBufferPos();
	void read(void *data, int size);
	void write(const void *data, int size);
	uint32 readLEUint32();
	int32 readLESint32();
	bool readLEBool();
	byte readByte();
	void writeLEUint32(uint32 data);
	void writeLESint32(int32 data);
	void writeLEBool(bool data);
	void writeByte(byte data);

protected:
	bool _saving;
	Common::InSaveFile *_inSaveFile;
	Common::OutSaveFile *_outSaveFile;
	uint32 _currentSection;
	uint32 _sectionSize;
	uint32 _sectionPtr;
	byte *_sectionBuffer;
};

} // end of namespace Grim

#endif
