// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QStandardPaths>
#include <QDir>

#include "datacoordinator.h"
#include "gui/databaseui.h"

#include <QDebug>

DatabaseUI *ui;

static void
uiLog(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
	switch (type)
	{
	case QtDebugMsg:
	case QtWarningMsg:
	case QtCriticalMsg:
		ui->writeLog(msg);
		break;
	case QtFatalMsg:
		ui->writeLog(msg);
		abort();
	}
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	// cd into the folder which contains the database file
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
	QDir dir;
	dir.mkpath(dataPath);
	QDir::setCurrent(dataPath);

	// Initialize GUI and direct log entries into it
	ui = new DatabaseUI;
	qInstallMessageHandler(uiLog);

	// Initialize and link up other components
	QNetworkAccessManager netAccessManager;
//	QNetworkCookieJar     netCookieJar;
	DataCoordinator       dataCoordinator;

	ui->setDbModel(dataCoordinator.dbModel());
//	netAccessManager.setCookieJar(&netCookieJar);
	dataCoordinator.setNetworkAccessManager(&netAccessManager);

	// Handle signals from the GUI
	QObject::connect(ui, &DatabaseUI::refreshDbRequested,
			&dataCoordinator, &DataCoordinator::refreshDatabase);
	QObject::connect(ui, &DatabaseUI::forceRederiveRequested,
			&dataCoordinator, &DataCoordinator::forceRederiveData);
	QObject::connect(ui, &DatabaseUI::exportRequested,
			&dataCoordinator, &DataCoordinator::exportData);

	// Good to go!
	ui->show();

	return a.exec();
}
