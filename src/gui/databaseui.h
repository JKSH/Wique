// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef DATABASEUI_H
#define DATABASEUI_H

#include <QWidget>
class QAbstractTableModel;
class QSortFilterProxyModel;


namespace Ui {
class DatabaseUI;
}

class DatabaseUI : public QWidget
{
	Q_OBJECT

signals:
	void refreshDbRequested() const;
	void forceRederiveRequested() const;
	void exportRequested(const QString& exportDir) const;

public:
	explicit DatabaseUI(QWidget* parent = nullptr);
	~DatabaseUI();

	void setDbModel(QAbstractTableModel* model);
	void writeLog(const QString& message);

private:
	Ui::DatabaseUI *ui;

	QSortFilterProxyModel* modelFilter;
};

#endif // DATABASEUI_H
