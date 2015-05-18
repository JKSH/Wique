// Copyright (c) 2015 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.MIT for details)

#include "spreadsheetview.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QKeyEvent>

/**********************************************************************\
 * PROTECTED
\**********************************************************************/
void
SpreadsheetView::keyPressEvent(QKeyEvent *event)
{
	if (event->matches(QKeySequence::Copy))
		copySelectedText();
	else
		QTableView::keyPressEvent(event);

	// TODO: Implement delete/cut/paste
}

/**********************************************************************\
 * PRIVATE
\**********************************************************************/
void
SpreadsheetView::copySelectedText() const
{
	QItemSelection sel = selectionModel()->selection();
	if (sel.size() == 0)
		return;

	// Find the bounding rectangle of the selection
	int top    = sel[0].top();
	int bottom = sel[0].bottom();
	int left   = sel[0].left();
	int right  = sel[0].right();
	for (int i = 1; i < sel.size(); ++i)
	{
		top    = std::min(top,    sel[i].top());
		bottom = std::max(bottom, sel[i].bottom());
		left   = std::min(left,   sel[i].left());
		right  = std::max(right,  sel[i].right());
	}

	QModelIndexList cells = sel.indexes();
	int width  = right-left+1;
	int height = bottom-top+1;


	// If selection is rectangular...
	if ( width*height == cells.count() )
	{
		std::sort(cells.begin(), cells.end());

		QString text;
		int i = 0;
		for (int row = 0; row < height; ++row)
		{
			// Add data to flattened string.
			// Text that contains '\n' must be replaced.
			for (int col = 0; col < width; ++col)
				text += cells[i++].data().toString().replace('\n', '\r') + '\t';

			text.chop(1); // Remove last '\t'
			text += '\n';
		}
		// Note: Both Microsoft Excel 2013 and LibreOffice Calc 4 keep the last '\n'

		QGuiApplication::clipboard()->setText(text);
	}
}
