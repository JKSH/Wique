// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QJsonArray>

class Database : public QObject
{
	Q_OBJECT

public:
	Database(QObject* parent = nullptr);
	~Database() { _db.close(); }

	QVector<int> allPageIds() const;
	int idOf(const QString& title) const;
	QString lastModified(int pageId) const;
	void exportWikiText(const QString& exportDir) const;
	void updateDatabase(const QJsonArray& wikiData);
	void deletePages(const QVector<int>& pageIds);

	void deepScanForRedirects();
	QAbstractTableModel* dbModel() const {return _model;}

private:
	void updateModel();
	QString extractRedirection(const QString& wikiText) const;

	QSqlDatabase _db;
	QSqlQueryModel* _model;
};

#endif // DATABASE_H
