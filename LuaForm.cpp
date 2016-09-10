#include "LuaForm.h"
#include "ui_LuaForm.h"

#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QFile>
#include <QFont>
#include <QScrollBar>
#include <QDebug>

#include "LuaHighlighter.h"


LuaForm::LuaForm(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::LuaForm)
{
	m_ui->setupUi( this );
	m_ui->plainTextOutput->setReadOnly( true );
	new LuaHighlighter( m_ui->sourceEdit->document() );

	connect( m_ui->sourceEdit, &CodeEditor::textChanged, this, &LuaForm::modified );
	connect( m_ui->sourceEdit, &CodeEditor::requestSave, this, &LuaForm::on_buttonSave_clicked );

	m_vm = new LuaThread( this );
	connect( m_vm, &LuaThread::fromStdOut, this, &LuaForm::vm_stdout );


	m_ui->buttonStop->setEnabled( false );
	connect( m_vm, &LuaThread::started, [this]{
		m_ui->buttonStop->setEnabled( true );
		m_ui->buttonStart->setEnabled( false );
	} );
	connect( m_vm, &LuaThread::stopped, [this]{
		m_ui->buttonStart->setEnabled( true );
		m_ui->buttonStop->setEnabled( false );
	} );


	QFont font( QLatin1String( "monospace" ) );
#ifdef _WIN32
	font.setFamily( QLatin1String( "Courier New" ) );
#endif

	QSettings settings;
	settings.beginGroup( QLatin1String( "lua" ) );
	font.fromString( settings.value( QLatin1String( "font" ), font.toString() ).toString() );
	m_ui->splitter->restoreState( settings.value( QLatin1String( "splitter" ), m_ui->splitter->saveState() ).toByteArray() );
	settings.endGroup();

	setFont( font );
}

LuaForm::~LuaForm()
{
	QSettings settings;
	settings.beginGroup( QLatin1String( "lua" ) );
	settings.setValue( QLatin1String( "font" ), m_ui->plainTextOutput->font().toString() );
	settings.setValue( QLatin1String( "splitter" ), m_ui->splitter->saveState() );
	settings.endGroup();

	delete m_ui;
}


void LuaForm::setFont( QFont const& font )
{
	m_ui->sourceEdit->setFont( font );
	m_ui->plainTextOutput->setFont( font );

	QFontMetrics metrics( font );
	int w = metrics.width( QLatin1Char( ' ' ) ) * 4;

	m_ui->sourceEdit->setTabStopWidth( w );
	m_ui->plainTextOutput->setTabStopWidth( w );
}


void LuaForm::vm_stdout( QString const& msg )
{
	m_ui->plainTextOutput->moveCursor( QTextCursor::End );
	m_ui->plainTextOutput->insertPlainText( msg );
	auto vsb = m_ui->plainTextOutput->verticalScrollBar();
	vsb->setValue( vsb->maximum() );
}

void LuaForm::on_buttonOpen_clicked()
{
	QSettings s;
	s.beginGroup( QLatin1String( "lua" ) );

	QString f = m_filename;
	if( f.isEmpty() )
	{
		f = s.value( QLatin1String( "file_lua" ), QString() ).toString();
	}
	f = QFileDialog::getOpenFileName( this, QLatin1String( "Open File" ), f, QLatin1String( "*.lua" ) );
	if( ! f.isEmpty() )
	{
		QFile file( f );
		if( file.open( QFile::ReadOnly ) )
		{
			m_filename = f;
			m_ui->sourceEdit->setPlainText( QString::fromLocal8Bit( file.readAll() ) );
			m_ui->sourceEdit->document()->clearUndoRedoStacks();

			s.setValue( QLatin1String( "file_lua" ), QFileInfo( f ).absoluteDir().path() );

			emit saved();
			emit filename( f );
		}
	}
}

void LuaForm::on_buttonSaveAs_clicked()
{
	QSettings s;
	s.beginGroup( QLatin1String( "lua" ) );

	QString f = m_filename;
	if( f.isEmpty() )
	{
		f = s.value( QLatin1String( "file_lua" ), QString() ).toString();
	}
	f = QFileDialog::getSaveFileName( this, QLatin1String( "Save File" ), f, QLatin1String( "*.lua" ) );
	if( ! f.isEmpty() )
	{
		QFile file( f );
		if( file.open( QFile::WriteOnly | QFile::Truncate ) )
		{
			m_filename = f;
			file.write( m_ui->sourceEdit->toPlainText().toLocal8Bit() );
			s.setValue( QLatin1String( "file_lua" ), QFileInfo( f ).absoluteDir().path() );

			emit saved();
			emit filename( f );
		}
	}
}


void LuaForm::on_buttonSave_clicked()
{
	QFile file( m_filename );
	if( !m_filename.isEmpty() && file.open( QFile::WriteOnly | QFile::Truncate ) )
	{
		file.write( m_ui->sourceEdit->toPlainText().toLocal8Bit() );

		emit saved();
		emit filename( m_filename );
	}
	else
	{
		on_buttonSaveAs_clicked();
	}
}


void LuaForm::on_buttonStart_clicked()
{
	if( ! m_vm->isRunning() )
	{
		m_vm->setScript( m_ui->sourceEdit->toPlainText() );
		m_vm->setSearchDirs( QFileInfo( m_filename ).absoluteDir().absolutePath() );
		m_vm->start();
	}
}


void LuaForm::on_buttonStop_clicked()
{
	m_vm->stop();
}

void LuaForm::on_buttonFont_clicked()
{
	QSettings s;
	s.beginGroup( QLatin1String( "lua" ) );

	QFont font;
	font.fromString( s.value( QLatin1String( "font" ), QString() ).toString() );

	bool ok;
	font = QFontDialog::getFont( &ok, font, this, tr( "Select Font" ) );

	if( ok )
	{
		setFont( font );
		s.setValue( QLatin1String( "font" ), font.toString() );
	}

	s.endGroup();
}
