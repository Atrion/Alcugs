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

#include <QCryptographicHash>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStringList>
#include <stdexcept>

#include "checksum-cache.h"

extern QTextStream log;

const uint version = 1;

ChecksumCache::ChecksumCache(QString filename) : filename(filename)
{
	// load from file
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return; // could be that the file does not exist at all
	QTextStream text;
	text.setDevice(&file);
	uint versionKontrolle = 0;
	if (!text.atEnd()) versionKontrolle = text.readLine().toUInt();
	if (!versionKontrolle || versionKontrolle != version) return;
	// get data
	while (!text.atEnd()) {
		QString line = text.readLine();
		QStringList lineSplit = line.split("|");
		if (lineSplit.size() != 3) continue;
		FileData data;
		data.checksum = lineSplit[1];
		data.time = lineSplit[2].toUInt();
		QString fileName = lineSplit[0];
		if (data.time && data.checksum.size() && file.size())
			cache[fileName] = data;
	}
}

ChecksumCache::~ChecksumCache(void)
{
	// we have to save the cache to the file
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) throw std::runtime_error("Could not write cache file");
	QTextStream text;
	text.setDevice(&file);
	text << version << "\n";
	// put the data
	foreach (const QString &str, cache.keys()) {
		FileData data = cache[str];
		text << str << "|" << data.checksum << "|" << data.time << "\n";
	}
}

bool ChecksumCache::checkFileChecksum(QString file, QString sum, uint size, QString /*options*/)
{
	QFile realFile(file);
	QFileInfo realInfo(file);
	// check existance
	if (!realFile.open(QIODevice::ReadOnly)) {
		log << file << " doesn't exist at all\n";
		return false;
	}
	// check size
	if (!size || size != realInfo.size()) {
		log << "File size of " << file << " does not match.\n";
		return false;
	}
	// check if we have this in the cache
	if (cache.contains(file)) {
		FileData data = cache[file];
		if (data.checksum == sum && data.time == realInfo.lastModified().toTime_t()) {
			// we found the file in our cache, it seems to be untouched!
			return true;
		}
	}
	// no luck, we have to re-calculate
	QByteArray realSumBytes = QCryptographicHash::hash(realFile.readAll(), QCryptographicHash::Md5);
	QString realSum = QString(realSumBytes.toHex());
	if (QString::compare(realSum, sum, Qt::CaseInsensitive) != 0) {
		log << file << " has the wrong checksum\n";
		return false;
	}
	// we got a correct checksum - let's put that into our cache
	QFileInfo info(file);
	FileData data;
	data.checksum = sum;
	data.time = info.lastModified().toTime_t();
	cache[file] = data;
	return true;
}
