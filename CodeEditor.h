#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QTextEdit>
#include <QObject>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;


class CodeEditor : public QTextEdit
{
	Q_OBJECT

	public:

		CodeEditor(QWidget *parent = 0);

		void lineNumberAreaPaintEvent(QPaintEvent *event);
		int lineNumberAreaWidth();

	signals:

		void requestSave( void );

	protected:

		virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
		virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
		virtual void changeEvent(QEvent* e ) Q_DECL_OVERRIDE;

		QTextBlock findFirstVisibleBlock( void );

	public slots:

		bool increaseSelectionIndent( void );

	private slots:

		void updateLineNumberAreaWidth(int newBlockCount);
		void highlightCurrentLine();
		void updateLineNumberArea();

	private:

		QWidget *lineNumberArea;
};


class LineNumberArea : public QWidget
{
	public:
		LineNumberArea(CodeEditor *editor) : QWidget(editor) {
			codeEditor = editor;
		}

		QSize sizeHint() const Q_DECL_OVERRIDE {
			return QSize(codeEditor->lineNumberAreaWidth(), 0);
		}

	protected:
		void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE {
			codeEditor->lineNumberAreaPaintEvent(event);
		}

	private:
		CodeEditor *codeEditor;
};


#endif
