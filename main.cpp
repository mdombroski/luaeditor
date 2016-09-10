#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	a.setApplicationName( "LuaEditor" );
	a.setApplicationVersion( "0.1" );
	a.setOrganizationDomain( "mattski.net.nz" );

	MainWindow w;
	w.show();

	return a.exec();
}
