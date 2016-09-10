#ifndef LUAFORM_H
#define LUAFORM_H

#include <QWidget>

#include "LuaThread.h"

class QFont;

namespace Ui {
	class LuaForm;
}

class LuaForm : public QWidget
{
	Q_OBJECT

	public:

		explicit LuaForm(QWidget *parent = 0);
		~LuaForm();

		LuaThread* controller( void ) const
		{
			return m_vm;
		}

		void setFont( QFont const& font );

	signals:

		void modified( void );
		void saved( void );

		void filename( QString const& );

	private slots:

		void vm_stdout( QString const& msg );

		void on_buttonOpen_clicked();
		void on_buttonSaveAs_clicked();
		void on_buttonStart_clicked();
		void on_buttonStop_clicked();

		void on_buttonFont_clicked();

		void on_buttonSave_clicked();

	private:

		Ui::LuaForm* m_ui;
		QString m_filename;

		LuaThread* m_vm;
};

#endif // LUAFORM_H
