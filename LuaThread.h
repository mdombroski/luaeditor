#ifndef LUATHREAD_H
#define LUATHREAD_H


#include <QObject>


class LuaThread : public QObject
{
	Q_OBJECT

	struct pi_State;
	friend class pi_State;

	public:

		LuaThread( QObject* parent = 0 );
		~LuaThread( void );

		bool wait( unsigned s = 10000 );
		void terminate( void );

		bool isRunning( void );

	protected:

		void thread( void );

	signals:

		void started( void );
		void stopped( void );

		void fromStdOut( QString const& txt );
		void currentLine( int n );

	public slots:

		void start( void );
		void stop( void );

		void setSearchDirs( QString const& dir );
		void setSearchDirs( QStringList const& dirs );

		void setScript( QString const& text );

	private slots:

		void pipe_rx( void );

	private:

		pi_State *m_state;
};

#endif // LUATHREAD_H
