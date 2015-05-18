// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "wikiquerier.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

// TODO: Extract error messages from MediaWiki replies (e.g. if querying fails)

// TODO: Allow user-configurable URL
static const QString apiUrl = "https://wiki.qt.io/api.php";

/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
WikiQuerier::WikiQuerier(QObject* parent) :
	QObject(parent),
	nam(nullptr),
	isBusy(false)
{
}

/**********************************************************************\
 * PUBLIC
\**********************************************************************/
void
WikiQuerier::queryPageList()
{
	if (isBusy)
	{
		qDebug() << "ERROR: WikiQuerier is busy.";
		return;
	}

	isBusy = true;
	namespaceListIdx = 0;
	_tmp_allIds.clear();
	fetchPageListChunk();
}

void
WikiQuerier::queryLastModified(const QVector<int>& pageIds)
{
	if (isBusy)
	{
		qDebug() << "ERROR: WikiQuerier is busy.";
		return;
	}
	if (pageIds.isEmpty())
		return;

	qDebug() << "(4) Fetching page timestamps...";

	isBusy = true;
	idChunkIdx = 0;
	_tmp_allIds_chunked.clear();
	_tmp_allTimestamps.clear();
	for (int i = 0; i < pageIds.count(); i += 50)
		_tmp_allIds_chunked << pageIds.mid(i, 50);
	fetchTimestampChunk(_tmp_allIds_chunked[0]);
}

void
WikiQuerier::downloadPages(const QVector<int>& pageIds)
{
	qDebug() << "Downloading" << pageIds.count() << "pages...";
	if (isBusy)
	{
		qDebug() << "ERROR: WikiQuerier is busy.";
		return;
	}
	if (pageIds.isEmpty())
		return;

	isBusy = true;
	textChunkIdx = 0;
	_tmp_texts_chunked.clear();
	_tmp_texts = QJsonArray();
	for (int i = 0; i < pageIds.count(); i += 50)
		_tmp_texts_chunked << pageIds.mid(i, 50);
	fetchTextChunk(_tmp_texts_chunked[0]);
}

/**********************************************************************\
 * PRIVATE
\**********************************************************************/
static QVector<int>
namespaceIdList{
	0,  // Base
	4,  // Project (Qt Wiki)
	12, // Help
	14  // Category
};

void
WikiQuerier::fetchPageListChunk(int namespaceId, const QString& apcontinue)
{
	QUrlQuery query;
	query.addQueryItem("format",      "json");
	query.addQueryItem("action",      "query");
	query.addQueryItem("list",        "allpages");
	query.addQueryItem("apnamespace", QString::number(namespaceId));
	query.addQueryItem("aplimit",     "500"); // Non-bots that query using pageid/title are limited to 500 pages at a time
	if (!apcontinue.isEmpty())
		query.addQueryItem("apfrom", apcontinue.toUtf8().toPercentEncoding());

	QUrl fullUrl(apiUrl);
	fullUrl.setQuery(query);
	QNetworkRequest netRequest(fullUrl);
	netRequest.setRawHeader("User-Agent", "Wique 0.5");
//	netRequest.setHeader(  QNetworkRequest::CookieHeader, qVariantFromValue( nam->cookieJar()->cookiesForUrl(fullUrl) )  );

	QNetworkReply* reply = nam->get(netRequest);
	connect(reply, &QNetworkReply::finished, [=]
	{
		auto outerObj = QJsonDocument::fromJson(reply->readAll()).object();
		reply->deleteLater();

		qDebug() << "\t1 page list chunk obtained";

		if (!outerObj.contains("query"))
		{
			qDebug() << "ERROR: WikiQuerier: Query failed. Raw reply is" << outerObj;
			return;
			// TODO: Signal that the refresh operation has finished (albeit prematurely)
		}

		bool continuingHere = outerObj.contains("query-continue");
		bool continuingNext;
		if (continuingHere)
		{
			// Kick off the next set of downloads
			QString next = outerObj["query-continue"].toObject()["allpages"].toObject()["apcontinue"].toString();
			fetchPageListChunk(namespaceIdList[namespaceListIdx], next);
		}
		else
		{
			// Kick off the next set of downloads
			continuingNext = ++namespaceListIdx < namespaceIdList.count();
			if (continuingNext)
				fetchPageListChunk(namespaceIdList[namespaceListIdx]);
		}

		// Actual processing
		auto innerArray = outerObj["query"].toObject()["allpages"].toArray();
		for (const QJsonValue& pageObj : innerArray)
			_tmp_allIds << pageObj.toObject()["pageid"].toInt();

		if (!continuingHere && !continuingNext)
		{
			// ASSUMPTION: The downloaded list is only ever for detailed updates
			qDebug() << "...Found" << _tmp_allIds.count() << "pages in total.\n";

			isBusy = false;
			emit pageListFetched(_tmp_allIds);
		}
	});
	// TODO: Handle network errors
}

void
WikiQuerier::fetchTimestampChunk(QVector<int> ids)
{
	QStringList idStrings;
	for (int id : ids)
		idStrings << QString::number(id);


	QUrlQuery query;
	query.addQueryItem("format",  "json");
	query.addQueryItem("action",  "query");
	query.addQueryItem("prop",    "info");
	query.addQueryItem("pageids",  idStrings.join('|')); // NOTE: Limited to 50 (or 500 for bots)

	QUrl fullUrl(apiUrl);
	fullUrl.setQuery(query);
	QNetworkRequest netRequest(fullUrl);
	netRequest.setRawHeader("User-Agent", "Wique 0.5");
//	netRequest.setHeader(  QNetworkRequest::CookieHeader, qVariantFromValue( nam->cookieJar()->cookiesForUrl(url_query) )  );

	auto reply = nam->get(netRequest);
	connect(reply, &QNetworkReply::finished, [=]
	{
		auto outerObj = QJsonDocument::fromJson(reply->readAll()).object();
		reply->deleteLater();

		qDebug() << "\t1 timestamp chunk obtained";

		if (!outerObj.contains("query"))
		{
			qDebug() << "Query failed. Raw reply:" << outerObj;
			return;
			// TODO: Signal that the refresh operation has finished (albeit prematurely)
		}

		// Kick off the next set of downloads
		bool continuing = ++idChunkIdx < _tmp_allIds_chunked.count();
		if (continuing)
			fetchTimestampChunk(_tmp_allIds_chunked[idChunkIdx]);

		// Actual processing
		auto innerObj = outerObj["query"].toObject()["pages"].toObject();
		for (const QString& key : innerObj.keys())
		{
			auto timeStr = innerObj[key].toObject()["touched"].toString();
			_tmp_allTimestamps[key.toInt()] = timeStr;
		}

		if (!continuing)
		{
			// ASSUMPTION: The downloaded list is only ever for detailed updates
			qDebug() << "...Found" << _tmp_allTimestamps.count() << "timestamps in total.\n";
			isBusy = false;
			emit timestampsFetched(_tmp_allTimestamps);
		}
	});
	// TODO: Handle network errors
}

void
WikiQuerier::fetchTextChunk(QVector<int> pageIds)
{
	if (pageIds.isEmpty())
		return; // TODO: Notify that fetching is over

	QStringList idStrings;
	for (int id : pageIds)
		idStrings << QString::number(id);

	QUrlQuery query;
	query.addQueryItem("format",  "json");
	query.addQueryItem("action",  "query");
	query.addQueryItem("prop",    "info|revisions");
	query.addQueryItem("rvprop",  "content");
	query.addQueryItem("pageids", idStrings.join('|'));

	// TODO: Consider whether to process in chunks, or one page at a time
	// DECISION: Implement chunk, do the one-page function in terms of the chunked function

	// TODO: Decide how to emit signals

	QUrl url(apiUrl);
	url.setQuery(query);

	QNetworkRequest netRequest(url);
	netRequest.setRawHeader("User-Agent", "Wique 0.5");
//	netRequest.setHeader(  QNetworkRequest::CookieHeader, qVariantFromValue( nam->cookieJar()->cookiesForUrl(url) )  );

	auto reply = nam->get(netRequest);
	connect(reply, &QNetworkReply::finished, [=]
	{
		auto outerObj = QJsonDocument::fromJson(reply->readAll()).object();
		reply->deleteLater();

		if (!outerObj.contains("query"))
		{
			qDebug() << "Query failed. Raw reply:" << outerObj;
			return;
			// TODO: Signal that the operation has finished (albeit prematurely)
		}

		// Kick off the next set of downloads before processing local data
		bool continuing = ++textChunkIdx < _tmp_texts_chunked.count();
		if (continuing)
			fetchTextChunk(_tmp_texts_chunked[textChunkIdx]);

		// Actual processing
		auto midObj = outerObj["query"].toObject()["pages"].toObject();
		for (const QString& key : midObj.keys())
		{
			auto pageObj = midObj[key].toObject();
			auto innerArray = pageObj["revisions"].toArray();
			if (innerArray.isEmpty())
			{
				qDebug() << "ERROR: Missing revisions";
				continue;
			}

			QJsonObject dataObj;
			dataObj["pageid"] = pageObj["pageid"].toInt();
			dataObj["title"] = pageObj["title"].toString();
			dataObj["touched"] = pageObj["touched"].toString();
			dataObj["content"] = innerArray[0].toObject()["*"].toString();

			_tmp_texts << dataObj;
		}

		if (!continuing)
		{
			qDebug() << "\t1 text chunk obtained";
			emit wikiTextFetched(_tmp_texts);
		}
	});
	// TODO: Handle network errors
}
