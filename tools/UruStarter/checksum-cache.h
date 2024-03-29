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

#ifndef CHECKSUM_CACHE_H
#define CHECKSUM_CACHE_H

#include <QString>
#include <QMap>

class ChecksumCache
{
public:
	ChecksumCache(QString filename);
	~ChecksumCache(void);
	
	bool checkFileChecksum(QString file, QString sum, uint size, QString options);
private:
	QString filename;
	
	struct FileData {
		QString checksum;
		uint time;
	};
	typedef QMap<QString, FileData> FileMap;
	
	FileMap cache;
};

#endif
