#ifndef LUAHIGHLIGHTER_H
#define LUAHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

class LuaHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	public:
		LuaHighlighter(QTextDocument *parent = 0);

	protected:
		void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

	private:
		struct HighlightingRule
		{
			QRegExp pattern;
			QTextCharFormat format;
		};
		QVector<HighlightingRule> highlightingRules;

		QRegExp commentStartExpression;
		QRegExp commentEndExpression;
		QRegExp quoteStartExpression;
		QRegExp quoteEndExpression;

		QTextCharFormat keywordFormat;
		QTextCharFormat valueFormat;
		QTextCharFormat singleLineCommentFormat;
		QTextCharFormat quotationFormat;
		QTextCharFormat functionFormat;
};

#endif // LUAHIGHLIGHTER_H
