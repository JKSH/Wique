// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "databaseui.h"
#include "ui_databaseui.h"
#include <QFileDialog>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
DatabaseUI::DatabaseUI(QWidget* parent) :
	QWidget(parent),
	ui(new Ui::DatabaseUI),
	modelFilter(new QSortFilterProxyModel(this))
{
	ui->setupUi(this);
	ui->table_dbView->setModel(modelFilter);

	modelFilter->setFilterKeyColumn(3);
	connect(ui->lineEdit_titleFilter, &QLineEdit::textChanged, [=](const QString& text)
	{
		modelFilter->setFilterRegExp(text);
	});
	connect(ui->button_exportData, &QPushButton::clicked, [=]
	{
		QString exportDir = QFileDialog::getExistingDirectory(this, "Select export directory");
		if (exportDir.isEmpty())
			return;
		emit exportRequested(exportDir);
	});

	connect(ui->button_refreshDb, &QPushButton::clicked,
			this, &DatabaseUI::refreshDbRequested);
	connect(ui->button_forceScanForRedirections, &QPushButton::clicked,
			this, &DatabaseUI::forceRederiveRequested);
}

DatabaseUI::~DatabaseUI()
{
	delete ui;
}

/**********************************************************************\
 * PUBLIC
\**********************************************************************/
void
DatabaseUI::setDbModel(QAbstractTableModel* model)
{
	modelFilter->setSourceModel(model);
}

void
DatabaseUI::writeLog(const QString &message)
{
	ui->textEdit_log->append(message);
}
