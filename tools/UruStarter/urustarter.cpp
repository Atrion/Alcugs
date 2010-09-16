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
#include <QApplication>
#include <QMessageBox>
#include <QLabel>
#include <QDesktopWidget>
#include <stdexcept>

#include "checksum-cache.h"
#include "resolution-patcher.h"

QTextStream log;
QLabel *display;

bool removeFile(QString file)
{
	QFile removeFile(file);
	if (!removeFile.exists()) return true;
	if (removeFile.remove()) {
		log << "Removed file: " << file << "\n";
		return true;
	}
	else {
		throw std::runtime_error(("Failed removing file: " + file).toStdString());
	}
}

void cleanDirectory(const QStringList &whitelist, QString dir, bool warnOnly = false)
{
	if (!dir.endsWith("/")) dir += "/";
	log << "Cleaning directory " << dir << "\n";
	if (dir == "/") dir = ""; // it may also be necessary to clean the root directory, indicated by an empty string
	// first, get the entries we are interested in
	QString file;
	QStringList list;
	QStringListIterator ite(whitelist);
	while (ite.hasNext()) {
		file = ite.next();
		if (file.startsWith(dir, Qt::CaseInsensitive) && file.lastIndexOf("/")+1 == dir.length())
			list << file;
	}
	if (list.size() == 0) {
		log << "WARNING: Found 0 whitelist entries for " << dir << " - not cleaning this directory (I would have to empty it)\n";
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

void setDisplayText(QString text)
{
	display->setText(" "+text);
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv); // for GUI stuff, but never used locally
	QStringList whitelist;
	// open logfile
	QFile logFile("log/UruStarter.log");
	if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(NULL, "Error while running UruStarter", "Could not open logfile");
		return 1;
	}
	log.setDevice(&logFile);
	log << "UruStarter initialized in " << QCoreApplication::applicationDirPath() << "\n";
	
	// open disaplay
	display = new QLabel("UruStarter is preparing your Uru installation...");
	display->setWindowFlags(Qt::Dialog);
	QPoint center = QDesktopWidget().availableGeometry().center();
	display->show();
	display->resize(display->width()+20, display->height()+20);
	display->move(center.x() - display->width()/2, center.y() - display->height()/2);
	setDisplayText(display->text()); // make sure it is shown
	
	try {
		// check if we are in an Uru installation and can write it
		if (!QFile::exists("UruExplorer.exe")) {
			throw std::runtime_error("This is NOT an Uru installation - UruExplorer.exe not found");
		}
		else {
			QFile uruExplorer("UruExplorer.exe");
			if (!uruExplorer.open(QIODevice::ReadWrite))
				throw std::runtime_error("I need full access to the Uru folder which I currently do not have. Please move your Uru installation to your user profile folder.");
		}
		
		{  // Process blacklist
			QFile blacklistFile("blacklist.txt");
			if (blacklistFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream listStream(&blacklistFile);
				while (!listStream.atEnd())
					removeFile(listStream.readLine());
			}
			// On Windows Vista and 7, try to write to the UruSetup.exe file - if that fails, the user obviously uses that file to start Uru, which he should not!
			if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0) {
				QFile uruSetup("UruSetup.exe");
				if (!uruSetup.open(QIODevice::ReadWrite)) {
					log << "It seems you are using the UruSetup.exe to start Uru\n";
					QMessageBox::warning(display, "Admin privileges", "You are currently using \"UruSetup.exe\" to start Uru. On Windows Vista and newer, "
						"this means that Uru will run with admin privileges. That is discouraged and can cause various problems.\n\nPlease use \"Uru.exe\", "
						"which can also be found in your Uru folder, to start Uru. Of course, you can also use a shortcut to that file.");
				}
			}
		}
		
		{ // do checksum checks, and put these files onto the Whitelist
			QFile checksumFile("whitelist-checksums.txt");
			if (checksumFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QStringList deletedAges;
				// start our cache
				ChecksumCache cache("urustarter-checksums.txt");
				
				QTextStream checkStream(&checksumFile);
				QString curAge;
				bool curAgeRequired = true;
				
				while (!checkStream.atEnd()) {
					QString line = checkStream.readLine();
					if (line.startsWith("#")) // skip comments
						continue;
					else if (line.startsWith("Age: ") || line.startsWith("AgeRequired: ")) {
						curAgeRequired = line.startsWith("AgeRequired: ");
						if (curAgeRequired) curAge = line.right(line.length()-13);
						else curAge = line.right(line.length()-5);
						setDisplayText("Checking "+curAge+"...");
						if (QFile::exists(curAge) && curAge.endsWith(".age"))
							log << "Doing checksum check for age " << curAge << "\n";
						else {
							if (curAgeRequired) {
								throw std::runtime_error(("The age "+curAge+" is not installed, but required to log in. "
										"Uru can not be started.").toStdString());
							}
							log << "Skipping checksum check for age " << curAge << "\n";
							curAge = QString(); // this age is not installed, we don't have to check it (won't be on the whitelist either)
						}
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
							log << "File " << file << " is invalid\n";
							if (curAgeRequired) {
								throw std::runtime_error(("The age "+curAge+" has an invalid version, but is required to log in. "
										"Uru can not be started.").toStdString());
							}
							removeFile(file);
							removeFile(curAge); // also remove the age file so that you can't link there
							deletedAges << curAge; // show it to the user
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
				
				if (!deletedAges.isEmpty()) {
					QMessageBox::information(display, "Ages removed", "The following ages were found to have an invalid version and were therefore removed"
						" from your Uru installation:\n\n"+deletedAges.join("\n"));
				}
			}
		}
		
		{ // process whitelist
			QFile listFile("whitelist.txt");
			if (listFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				setDisplayText("Cleaning up your client...");
				// process whitelist file
				const QString cleanDirPrefix = "cleandir:";
				QStringList cleanDirs;
				QTextStream listStream(&listFile);
				while (!listStream.atEnd()) {
					QString line = listStream.readLine();
					if (line.startsWith("#")) continue; // skip comments
					if (line.startsWith(cleanDirPrefix))
						cleanDirs << line.mid(cleanDirPrefix.length());
					else
						whitelist << line;
				}
				// add the whitelist file as it does not usually contain itself
				whitelist << "whitelist.txt";
				// clean directories
				if (cleanDirs.isEmpty())
					log << "WARNING: There is a whitelist.txt file, but no directory is set to be cleaned, so no cleanup will be done\n";
				else {
					QStringListIterator ite(cleanDirs);
					while (ite.hasNext())
						cleanDirectory(whitelist, ite.next());
				}
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
	}
	catch (std::exception &err) {
		log << err.what() << "\n";
		QMessageBox::critical(display, "Error while running UruStarter", err.what());
		return 1;
	}
	
	// close display, and be done
	delete display;
	return 0;
}
