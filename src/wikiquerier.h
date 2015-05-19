// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef WIKIQUERIER_H
#define WIKIQUERIER_H

#include <QObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>

class QNetworkAccessManager;

class WikiQuerier : public QObject
{
	Q_OBJECT
public:
	explicit WikiQuerier(QObject* parent = nullptr);

	void setNetworkAccessManager(QNetworkAccessManager* nam)
	{ this->nam = nam; }

	void queryPageList();
	void queryLastModified(const QVector<int>& pageIds);
	void downloadPages(const QVector<int>& pageIds);

signals:
	void pageListFetched(const QVector<int>& pageIds) const;
	void timestampsFetched(const QMap<int, QString>& timestampMap) const;
	void wikiTextFetched(const QJsonArray& data) const;

private:
	void fetchPageListChunk(int namespaceId = 0, const QString& apcontinue = QString());
	void fetchTimestampChunk(QVector<int> ids);
	void fetchTextChunk(QVector<int> pageIds);

	void finalizePageLists();
	void finalizeTimestamps();
	void finalizeWikiText();

	QNetworkAccessManager* nam;
	bool isBusy;
	int namespaceListIdx;

	// List of all IDs
	QVector<int> _tmp_allIds;

	// Temporaries for querying metadata
	QVector<QVector<int>> _tmp_allIds_chunked; // TODO: Calculate each iteration?
	int idChunkIdx;

	// Temporaries for storing timestamps
	QMap<int, QString> _tmp_allTimestamps;

	// Temporaries for querying page texts
	QVector<QVector<int>> _tmp_texts_chunked;
	int textChunkIdx;

	// Temporaries for storing page texts
	QJsonArray _tmp_texts;
};

#endif // WIKIQUERIER_H
