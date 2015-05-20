// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef DATACOORDINATOR_H
#define DATACOORDINATOR_H

#include <QObject>
#include "database.h"
#include "wikiquerier.h"

#include <QDebug>

class DataCoordinator : public QObject
{
	Q_OBJECT

signals:
	void currentJobFinished() const;

public:
	explicit DataCoordinator(QObject* parent = nullptr);

	void setNetworkAccessManager(QNetworkAccessManager* nam)
	{ wq->setNetworkAccessManager(nam); }

	void refreshDatabase();
	void forceRederiveData()
	{
		db->deepScanForRedirects();
		emit currentJobFinished();
	}

	void exportData(const QString& exportDir)
	{
		db->exportWikiText(exportDir);
		emit currentJobFinished();
	}

	QAbstractTableModel* dbModel() const
	{ return db->dbModel(); }

private:
	Database* db;
	WikiQuerier* wq;
};

#endif // DATACOORDINATOR_H
