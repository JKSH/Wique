// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "datacoordinator.h"

/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
DataCoordinator::DataCoordinator(QObject* parent) :
	QObject(parent),
	db(new Database(this)),
	wq(new WikiQuerier(this))
{
	connect(wq, &WikiQuerier::pageListFetched, [=](const QVector<int>& onlineIds)
	{
		// Don't delete anything if the list is incomplete
		if (!wq->lastOperationWasCompleted())
		{
			qDebug() << "(2) Skipping check for deleted pages.\n";
			qDebug() << "(3) Skipping deletion from database.\n";
			return;
		}

		qDebug() << "(2) Checking for deleted pages...";

		auto localIds = db->allPageIds().toList().toSet();
		auto removedIds = localIds - onlineIds.toList().toSet();
		if (removedIds.isEmpty())
		{
			qDebug() << "...No pages deleted.\n";
			qDebug() << "(3) Skipping deletion from database.\n";
			return;
		}
		qDebug() << "...Found" << removedIds.count() << "deleted pages.\n";

		qDebug() << "(3) Deleting pages from database...";
		db->deletePages(removedIds.toList().toVector());
		qDebug() << "...Done.\n";
	});
	connect(wq, &WikiQuerier::pageListFetched,
			wq, &WikiQuerier::queryLastModified);
	connect(wq, &WikiQuerier::timestampsFetched, [=](QMap<int,QString> stamps)
	{
		qDebug() << "(5) Checking for updates...";
		auto onlineIds =  stamps.keys();

		QVector<int> updatedIds;
		for (int id : onlineIds)
		{
			auto timestamp = db->lastModified(id);
			if (timestamp != stamps[id])
				updatedIds << id;
		}

		if (updatedIds.isEmpty())
		{
			qDebug() << "...No updates found.\n";
			qDebug() << "Database refresh completed.\n";
		}
		else
		{
			qDebug() << "...Found" << updatedIds.count() << "updates.\n";
			wq->downloadPages(updatedIds);
		}
	});
	connect(wq, &WikiQuerier::wikiTextFetched,
			db, &Database::updateDatabase);
}

/**********************************************************************\
 * PUBLIC
\**********************************************************************/
void
DataCoordinator::refreshDatabase()
{
	// This sets off an event-driven chain (see constructor):
	// 1. queryPageList()
	// 2. queryLastModified()
	// 3. downloadPages()
	qDebug() << "== Refreshing database ==";
	qDebug() << "(1) Fetching list of pages...";
	wq->queryPageList();
}
