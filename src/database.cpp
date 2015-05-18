// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "database.h"

#include <QApplication>
#include <QDir>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>

#include <QDebug>

/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
Database::Database(QObject* parent) :
	QObject(parent),
	_db(QSqlDatabase::addDatabase("QSQLITE")),
	_model(new QSqlQueryModel(this))
{
	_db.setDatabaseName("data.db");
	if (!_db.open())
	{
		qWarning() << "ERROR: Database: Failed to open data.db";
		return;
	}

	QString createPageTable =
			"CREATE TABLE IF NOT EXISTS Pages("
			"id INTEGER PRIMARY KEY,"

			// BUG? Must write "Pages(id)" instead of "id", or else Qt's SQLite driver will fail to prepare queries
			"redirection INTEGER REFERENCES Pages(id),"
			"title TEXT,"
			"timestamp TEXT,"
			"wikitext TEXT)";

	QSqlQuery q;
	if (!q.exec("PRAGMA foreign_keys = ON"))
		qWarning() << "ERROR: Database: Enabling foreign keys:" << q.lastError();
	if (!q.exec(createPageTable))
		qWarning() << "ERROR: Database: Creating table Pages:" << q.lastError();

	updateModel();
}


/**********************************************************************\
 * PUBLIC
\**********************************************************************/
QVector<int>
Database::allPageIds() const
{
	QSqlQuery q(_db);
	if (!q.exec("SELECT id FROM Pages"))
		qWarning() << "ERROR: Database: Loading all IDs:" << q.lastError();

	QVector<int> ids;
	while (q.next())
		ids << q.value("id").toInt();
	return ids;
}

int
Database::idOf(const QString& title) const
{
	QSqlQuery q;
	q.prepare("SELECT id FROM Pages WHERE title=:title");
	q.bindValue(":title", title);
	q.exec();

	if (q.next())
		return q.value("id").toInt();
	return -1;
}

QString
Database::lastModified(int pageId) const
{
	// NOTE: Storing QDateTime in QSQLITE is lossy :( Use QString instead

	QSqlQuery q;
	if (!q.prepare("SELECT timestamp FROM Pages WHERE id=:id"))
		qWarning() << "ERROR: Database: Binding timestamp query:" << q.lastError();
	q.bindValue(":id", pageId);
	if (!q.exec())
		qWarning() << "ERROR: Database: Loading timestamps:" << q.lastError();
	if (q.next())
		return q.value("timestamp").toString();
	return "";
}

void
Database::exportWikiText(const QString& exportDir) const
{
	QDir dir(exportDir);
	dir.mkdir("exports");
	dir.cd("exports");
	dir.absolutePath();

	QSqlQuery q(_db);
	if (!q.exec("SELECT title, wikitext FROM Pages"))
		qWarning() << "ERROR: Database: Loading all text:" << q.lastError();

	while (q.next())
	{
		QString title = q.value("title").toString();
		title.replace('/', "__");
		title.replace(':', "__"); // TODO: (Wiki) Fix weird Categories

		QFile file(dir.absolutePath()+"/"+title+".txt");
		if (!file.open(QFile::WriteOnly|QFile::Text))
		{
			qWarning() << "ERROR: Database: Cannot open file for" << title;
			continue;
		}

		file.write(q.value("wikitext").toString().toUtf8());
	}
}

void
Database::updateDatabase(const QJsonArray& wikiData)
{
	auto existingIds = allPageIds();

	// TODO: Validate data

	QSqlQuery q;
	q.exec("BEGIN");
	for (const QJsonValue& val : wikiData)
	{
		auto pageObj = val.toObject();

		int pageId = pageObj["pageid"].toInt();
		QString title = pageObj["title"].toString();
		QString timestamp = pageObj["touched"].toString();
		QString content = pageObj["content"].toString();

		int redirection = -1;
		if (content.startsWith("#REDIRECT"))
		{
			qDebug() << "Extracting redirect...";

			QString link = extractRedirection(content);
			redirection = idOf(link);
			if (link.isEmpty() || redirection == -1)
			{
				qWarning() << "...not found!:" << link;
				continue;
			}
			qDebug() << "...redirecting to" << redirection << link;
		}

		if (existingIds.contains(pageId))
		{
			if (!q.prepare("UPDATE Pages SET redirection=:redirection, title=:title, timestamp=:timestamp, wikitext=:wikitext WHERE id=:id"))
				qWarning() << "ERROR: Database: Preparing Update query:" << q.lastError();
		}
		else
		{
			if (!q.prepare("INSERT INTO Pages (id, redirection, title, timestamp, wikitext) VALUES(:id, :redirection, :title, :timestamp, :wikitext)"))
				qWarning() << "ERROR: Database: Preparing Insert query:" << q.lastError();
		}
		q.bindValue(":id", pageId);
		q.bindValue(":title", title);
		q.bindValue(":timestamp", timestamp);
		q.bindValue(":wikitext", content);
		if (redirection == -1)
			q.bindValue(":redirection", QVariant());
		else
			q.bindValue(":redirection", redirection);
		if (!q.exec())
			qWarning() << "ERROR: Database: Executing UPDATE/INSERT for page" << title << ":" << q.lastError();
	}
	q.exec("COMMIT");

	updateModel();
}

void
Database::deletePages(const QVector<int>& pageIds)
{
	QSqlQuery q;
	q.exec("BEGIN"); // TODO: Check if many deletions need to be in 1 transaction
	for (int id : pageIds)
	{
		q.prepare("DELETE FROM Pages WHERE id=:id");
		q.bindValue(":id", id);
		if (!q.exec())
			qWarning() << "ERROR: Database: Executing DELETE for page" << id << ":" << q.lastError();
	}
	q.exec("COMMIT");

	updateModel();
}

void
Database::deepScanForRedirects()
{
	qDebug("== Deep scanning for redirections ==");
	QSqlQuery q;
	q.exec("SELECT id,wikitext FROM Pages");

	QSqlQuery r;
	r.exec("BEGIN");
	while (q.next())
	{
		// This function takes quite a while.
		// Manually process events to keep the GUI responsive.
		qApp->processEvents();

		int id = q.value("id").toInt();
		QString wikiText = q.value("wikitext").toString();

		int redirection = -1;
		if (wikiText.startsWith("#REDIRECT"))
		{
			QString link = extractRedirection(wikiText);
			redirection = idOf(link);
			if (redirection == -1)
				qWarning() << link << "not found in the main table!";
		}
		r.prepare("UPDATE Pages SET redirection=:redirection WHERE id=:id");

		r.bindValue(":id", id);
		if (redirection == -1)
			r.bindValue(":redirection", QVariant());
		else
			r.bindValue(":redirection", redirection);

		if (!r.exec())
			qWarning() << "ERROR: Database: Updating/Inserting derived data for" << id;
	}
	r.exec("COMMIT");

	qDebug() << "Done";
	updateModel();
}

/**********************************************************************\
 * PRIVATE
\**********************************************************************/
void
Database::updateModel()
{
	// Refresh. Block modelAboutToBeReset()/modelReset() signals
	// to maintain the views' sort order and scroll position
	QSqlQuery q("SELECT id, redirection, timestamp, title FROM Pages");

	_model->blockSignals(true);
	_model->setQuery(q);
	while (_model->canFetchMore())
		_model->fetchMore();
	_model->blockSignals(false);
}

QString
Database::extractRedirection(const QString& wikiText) const
{
	if (!wikiText.startsWith("#REDIRECT"))
		return "";

	// ASSUMPTION: "#REDIRECT" is always followed by a wikilink
	QRegularExpression regex_wikiLink("\\[\\[(.*?)\\]\\]");
	auto match = regex_wikiLink.match(wikiText);
	if (!match.hasMatch())
		return "";

	QString link = match.captured(1);

	// Normalize title
	link.replace("_", " ");

	// Handle links to category pages
	if (link.startsWith(":Category"))
		link = link.mid(1);

	return link;
}
