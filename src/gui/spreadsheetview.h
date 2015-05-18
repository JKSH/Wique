// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#ifndef SPREADSHEETVIEW_H
#define SPREADSHEETVIEW_H

#include <QTableView>

class SpreadsheetView : public QTableView
{
	Q_OBJECT

public:
	SpreadsheetView(QWidget* parent = nullptr) : QTableView(parent) {}

protected:
	void keyPressEvent(QKeyEvent* event) override;

private:
	void copySelectedText() const;
};

#endif // SPREADSHEETVIEW_H
