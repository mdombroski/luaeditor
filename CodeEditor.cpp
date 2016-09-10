#include "CodeEditor.h"

#include <QtWidgets>
#include <QTextCursor>


CodeEditor::CodeEditor(QWidget *parent) : QTextEdit(parent)
{
	lineNumberArea = new LineNumberArea(this);

	connect( this->document(), &QTextDocument::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth );
	connect( this->verticalScrollBar(), &QScrollBar::valueChanged, this, &CodeEditor::updateLineNumberArea );
	connect( this, &QTextEdit::textChanged, this, &CodeEditor::updateLineNumberArea );
	connect( this, &QTextEdit::cursorPositionChanged, this, &CodeEditor::updateLineNumberArea );

	connect( this, &QTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine );

	updateLineNumberAreaWidth(0);
	highlightCurrentLine();
}


int CodeEditor::lineNumberAreaWidth()
{
	int digits = 1;

	int max = qMax(1, document()->blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}

	int space = 6 + fontMetrics().width(QLatin1Char('9')) * digits;

	return space;
}



void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}



void CodeEditor::updateLineNumberArea()
{
	// Make sure the sliderPosition triggers one last time the valueChanged() signal with the actual value !!!!
	verticalScrollBar()->setSliderPosition( verticalScrollBar()->sliderPosition() );

	// Since "QTextEdit" does not have an "updateRequest(...)" signal, we chose
	// to grab the imformations from "sliderPosition()" and "contentsRect()".
	// See the necessary connections used (Class constructor implementation part).

	updateLineNumberAreaWidth( 0 );

	int dy = verticalScrollBar()->sliderPosition();
	if( dy > -1 )
	{
		lineNumberArea->scroll( 0, dy );
	}

	QRect crect = contentsRect();
	lineNumberArea->update( 0, crect.y(), lineNumberArea->width(), crect.height() );
}



void CodeEditor::resizeEvent(QResizeEvent *e)
{
	QTextEdit::resizeEvent(e);

	QRect cr = contentsRect();
	lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


void CodeEditor::keyPressEvent(QKeyEvent *e)
{
	if( e->key() == Qt::Key_S )
	{
		if( e->modifiers() == Qt::ControlModifier )
		{
			e->accept();
			emit requestSave();
			return;
		}
	}

	if( e->key() == Qt::Key_Tab )
	{
		if( increaseSelectionIndent() )
		{
			e->accept();
			return;
		}
	}

	QTextEdit::keyPressEvent( e );
}


void CodeEditor::changeEvent( QEvent* e )
{
	QTextEdit::changeEvent( e );

	if( e->type() == QEvent::PaletteChange )
	{
		highlightCurrentLine();
	}
}


void CodeEditor::highlightCurrentLine()
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if( !isReadOnly() )
	{
		QTextEdit::ExtraSelection selection;

		QColor lineColor = palette().color( QPalette::Base ).darker( 110 );

		selection.format.setBackground( lineColor );
		selection.format.setProperty( QTextFormat::FullWidthSelection, true );
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append( selection );
	}

	setExtraSelections( extraSelections );
}


QTextBlock CodeEditor::findFirstVisibleBlock( void )
{
	QTextDocument* doc = document();
	QTextBlock block = doc->begin();

	QRect vp_box( viewport()->geometry() );
	QPoint trans( vp_box.x(), vp_box.y() - verticalScrollBar()->sliderPosition() );

	while( block != doc->end() )
	{
		QRect block_box = doc->documentLayout()->blockBoundingRect( block ).translated( trans ).toRect();
		if( vp_box.intersects( block_box ) )
		{
			break;
		}
		block = block.next();
	}

	return block;
}



void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	verticalScrollBar()->setSliderPosition(this->verticalScrollBar()->sliderPosition());

	QPainter painter( lineNumberArea );
	painter.fillRect( event->rect(), Qt::lightGray );
	painter.setPen( Qt::black );

	QTextDocument* doc = document();
	QAbstractTextDocumentLayout* layout = doc->documentLayout();

	QTextBlock block = findFirstVisibleBlock();
	int blockNumber = block.blockNumber();

	qreal top = viewport()->geometry().top()
				+ doc->documentMargin()
				- 5
				- verticalScrollBar()->sliderPosition()
				+ layout->blockBoundingRect( block ).top();

	QRectF box( 0, top, lineNumberArea->width() - 3, 1 );

	while( block.isValid() && box.top() <= event->rect().bottom() )
	{
		box.setHeight( layout->blockBoundingRect( block ).height() );

		if( block.isVisible() && box.bottom() >= event->rect().top() )
		{
			painter.drawText( box, Qt::AlignRight, QString::number( ++blockNumber ) );
		}

		block = block.next();
		box.translate( 0, box.height() );
	}
}


bool CodeEditor::increaseSelectionIndent( void )
{
	QTextCursor cursor( textCursor() );
	if( ! cursor.hasSelection() )
	{
		return false;
	}

	int start = cursor.anchor();
	int end = cursor.position();

	if( start > end )
	{
		qSwap( start, end );
	}

	// get count of blocks involved
	cursor.setPosition( end, QTextCursor::MoveAnchor );
	int eblock = cursor.block().blockNumber();
	cursor.setPosition( start, QTextCursor::MoveAnchor );
	int sblock = cursor.block().blockNumber();

	// single line of text, not block indent mode
	if( sblock == eblock )
	{
		cursor.insertText( QLatin1String( "\t" ) );
		return true;
	}

	int bcount = eblock - sblock + 1;

	// begin inserting tabs
	cursor.beginEditBlock();
	for( int i = 0; i < bcount; ++i )
	{
		cursor.movePosition( QTextCursor::StartOfBlock, QTextCursor::MoveAnchor );
		cursor.insertText( QLatin1String( "\t" ) );
		cursor.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor );
	}
	cursor.endEditBlock();

	// reselect the indented block
	cursor.setPosition( start, QTextCursor::MoveAnchor );
	cursor.movePosition( QTextCursor::StartOfBlock, QTextCursor::MoveAnchor );
	while( cursor.block().blockNumber() < eblock )
	{
		cursor.movePosition( QTextCursor::NextBlock, QTextCursor::KeepAnchor );
	}
	cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );

	setTextCursor( cursor );

	return true;
}

