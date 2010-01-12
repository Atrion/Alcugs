/*******************************************************************************
*    UruStarter                                                                *
*    Copyright (C) 2009-2010  Diafero <diafero AT arcor.de>                    *
*                                                                              *
*    This program is free software; you can redistribute it and/or modify      *
*    it under the terms of the GNU General Public License as published by      *
*    the Free Software Foundation; either version 2 of the License, or         *
*    (at your option) any later version.                                       *
*                                                                              *
*    This program is distributed in the hope that it will be useful,           *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*    GNU General Public License for more details.                              *
*                                                                              *
*    You should have received a copy of the GNU General Public License         *
*    along with this program; if not, write to the Free Software               *
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                              *
*    Please see the file COPYING for the full license.                         *
*******************************************************************************/

#include <QFile>
#include <QTextStream>

extern QTextStream log;

static inline void seek(QFile *file, int size)
{
	file->seek(file->pos()+size);
}

static inline int getChar(QFile *file)
{
	char res;
	if (!file->getChar(&res)) {
		log << "Read past end of file in getChar\n";
		throw 0;
	}
	return res;
}

static inline int getInt(QFile *file)
{
	int res = getChar(file);
	res += getChar(file)*256;
	res += getChar(file)*256*256;
	res += getChar(file)*256*256*256;
	return res;
}

static inline void putInt(QFile *file, int val)
{
	file->putChar(val % 256); val /= 256;
	file->putChar(val % 256); val /= 256;
	file->putChar(val % 256); val /= 256;
	file->putChar(val % 256);
}

void patchResolution(int width, int height, int colourDepth)
{
	int controlValue;
	log << "Trying to set resolution to " << width << "x" << height << "\n";
	QFile file("dev_mode.dat");
	if (!file.open(QIODevice::ReadWrite)) {
		log << "Could not open dev_mode.dat\n";
		return;
	}
	
	// Open the file and stream to the position we need
	// skip device record
	controlValue = getInt(&file);
	if (controlValue != 11) {
		log << "Version is " << controlValue << ", should be 11\n";
		throw 0;
	}
	seek(&file, 2*4); // 3 uint32
	seek(&file, getInt(&file)); // a string
	seek(&file, getInt(&file)); // a string
	seek(&file, getInt(&file)); // a string
	seek(&file, getInt(&file)); // a string
	seek(&file, 8); // not sure about the length of this one - DirectX device caps
	seek(&file, 2*4); // 2 uint32
	controlValue = getInt(&file); // device mode count - always 0
	if (controlValue != 0) {
		log << "Device mode count is " << controlValue << ", should be 0\n";
		throw 0;
	}
	seek(&file, 5*4), // 5 floats
	seek(&file, 2*2*4); // 2 structs each containing 2 floats
	seek(&file, 2*1); // 2 uint8
	// now we are in the device mode, skip to resolution
	controlValue = getInt(&file); // device mode flags - always 0
	if (controlValue != 0) {
		log << "Device mode flags are " << controlValue << ", should be 0\n";
		throw 0;
	}
	// now this should be the resolution
	if (getInt(&file) != 800) {
		log << "Current width is not 800 - skipping resolution patching\n";
		return;
	}
	if (getInt(&file) != 600) {
		log << "Current height is not 600 - skipping resolution patching\n";
		return;
	}
	seek(&file, -8);
	// We got it, let's write :D
	putInt(&file, width);
	putInt(&file, height);
	log << "Resolution successfully set\n";
	// We might also have to change the colour depth
	if (colourDepth) {
		putInt(&file, colourDepth);
		log << "Colour depth successfully set\n";
	}
}
