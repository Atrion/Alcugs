/*******************************************************************************
*    UruStarter                                                                *
*    Copyright (C) 2008-2010  Diafero <diafero AT arcor.de>                    *
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

#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QSettings>

#include "checksum-cache.h"
#include "resolution-patcher.h"

QTextStream log;

bool removeFile(QString file)
{
	QFile removeFile(file);
	if (!removeFile.exists()) return true;
	if (removeFile.remove()) {
		log << "Removed file: " << file << "\n";
		return true;
	}
	else {
		log << "Failed removing file: " << file << "\n";
		return false;
	}
}

void cleanDirectory(const QStringList &whitelist, QString dir, bool warnOnly = false)
{
	if (!dir.endsWith("/")) dir += "/";
	log << "Cleaning directory " << dir << "\n";
	// first, get the entries we are interested in
	QString file;
	QStringList list;
	QStringListIterator ite(whitelist);
	while (ite.hasNext()) {
		file = ite.next();
		if (file.startsWith(dir, Qt::CaseInsensitive))
			list << file;
	}
	if (list.size() == 0) {
		log << "Found 0 whitelist entries for " << dir << " - not cleaning this directory\n";
		return;
	}
	
	// now get a list of all files in this directory
	QDir directory(dir);
	QStringList files = directory.entryList(QDir::Files);
	ite = QStringListIterator(files);
	while (ite.hasNext()) {
		// and check for each file if it is listed
		file = dir+ite.next();
		if (!list.contains(file, Qt::CaseInsensitive)) {
			// this file is not on the list - remvoe it
			if (warnOnly) {
				log << "Found file which is not on whitelist: " << file << "\n";
			} else {
				removeFile(file);
			}
		}
	}
}

int main(int argc, char **argv)
{
	QStringList whitelist;
	// open logfile
	QFile logFile("log/UruStarter.log");
	if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text))
		return 1;
	log.setDevice(&logFile);
	log << "UruStarter initialized\n";
	
	// check if we are in an Uru installation
	if (!QFile::exists("UruExplorer.exe")) {
		log << "This is NOT an Uru installation - UruExplorer.exe not found\n";
		return 1;
	}
	
	{  // Process blacklist
		QFile blacklistFile("blacklist.txt");
		if (blacklistFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream listStream(&blacklistFile);
			while (!listStream.atEnd())
				removeFile(listStream.readLine());
		}
	}
	
	{ // do checksum checks, and put these files onto the Whitelist
		QFile checksumFile("whitelist-checksums.txt");
		if (checksumFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			// start our cache
			ChecksumCache cache("urustarter-checksums.txt");
			
			QTextStream checkStream(&checksumFile);
			QString curAge;
			
			while (!checkStream.atEnd()) {
				QString line = checkStream.readLine();
				if (line.startsWith("#"))
					continue;
				else if (line.startsWith("Age: ")) {
					curAge = line.right(line.length()-5);
					if (QFile::exists(curAge) && curAge.endsWith(".age"))
						log << "Doing checksum check for age " << curAge << "\n";
					else
						curAge = QString(); // this age is not installed, we don't have to check it (won't be on the whitelist either)
				}
				else if (curAge.length()) {
					QStringList curFile = line.split("  ");
					if (curFile.size() != 2 && curFile.size() != 3) continue;
					QString sum = curFile[0], options, file = curFile[1];
					if (curFile.size() == 3) {
						options = curFile[1];
						file = curFile[2];
					}
					// now get content of sum
					curFile = sum.split(",");
					if (curFile.size() != 2) continue;
					uint size = curFile[1].toUInt();
					sum = curFile[0]; // that's the real checksum
					// files must all be in the dat directory
					if (!file.startsWith("dat/")) continue;
					// check checksum
					if (!cache.checkFileChecksum(file, sum, size, options)) {
						removeFile(file);
						removeFile(curAge); // also remove the age file so that you can't link there
						curAge = QString(); // don't put remaining files of this age on the whitelist - they will be removed
					}
					else {
						// the file is valid, add it to whitelist
						whitelist << file;
						if (file.endsWith(".age")) // add the sum file
							whitelist << (file.left(file.length()-3) + "sum");
					}
				}
			}
		}
	}
	
	{ // process whitelist
		QFile listFile("whitelist.txt");
		if (listFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream listStream(&listFile);
			while (!listStream.atEnd())
				whitelist << listStream.readLine();
			// clean directories
			cleanDirectory(whitelist, "python/system/");
			cleanDirectory(whitelist, "python/");
			cleanDirectory(whitelist, "sdl/");
			cleanDirectory(whitelist, "dat/");
			cleanDirectory(whitelist, "sfx/");
			cleanDirectory(whitelist, "img/");
		}
		else
			log << "Failed to load whitelist\n";
	}
	
	// do the widescreen patch - so, first load our settings
	{
		QSettings settings("urustarter.ini", QSettings::IniFormat);
		int width = settings.value("width").toInt();
		int height = settings.value("height").toInt();
		int colourDepth = settings.value("colourDepth").toInt();
		if (colourDepth != 0 && colourDepth != 16 && colourDepth != 32) {
			log << "Invalid colour depth: must be 16 or 32\n";
			colourDepth = 0;
		}
		if (height && width) {
			try {
				patchResolution(width, height, colourDepth);
			}
			catch (...) {
				log << "Unexpected error in the dev_mode.dat format - can not patch resolution\n";
			}
		}
	}
	
	if (argc > 1) {
		// Start Uru with the arguments we got (if we got any)
		QStringList arguments;
		for (int i = 1; i < argc; ++i) {
			arguments << argv[i];
		}
		QProcess *process = new QProcess;
		process->start("UruExplorer.exe", arguments);
		if (process->waitForStarted())
			log << "Successfully started Uru\n";
		else
			log << "Error starting Uru\n";
	}
	return 0;
}
