#include "LuaThread.h"

#include <lua.hpp>

#include <QDebug>
#include <QVariantMap>

#include <chrono>
#include <thread>
#include <cstdio>
#include <csetjmp>

#ifdef _WIN32
#include <mingw.thread.h>
#include <QWinEventNotifier>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <rpc.h>
#define fdopen _fdopen
#else
#include <QSocketNotifier>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif


struct LuaThread::pi_State
{
#ifdef _WIN32
	enum
	{
		buffer_size = 256
	};

	pi_State( LuaThread* parent ) :
		controller( parent ),
		thread( 0 )
	{
		pipe = "\\\\.\\pipe\\";

		// generate a random uuid for the pipe name
		{
			UUID id;
			RPC_CSTR str;
			UuidCreate( &id );
			UuidToStringA( &id, &str );
			pipe.append( (char*) str );
			RpcStringFreeA( &str );
		}

		prx = CreateNamedPipeA( pipe.data(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 2, 0, 256, 0, 0 );
		ptx = CreateFileA( pipe.data(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0 );

		memset( &ovrx, 0, sizeof(ovrx) );
		ovrx.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		restartrx();
	}

	~pi_State( void )
	{
		CancelIo( prx );
		CloseHandle( prx );
		CloseHandle( ptx );
	}

	void restartrx( void )
	{
		ReadFile( prx, buffer, buffer_size, 0, &ovrx );
	}

	QString readrx( void )
	{
		DWORD bytes;
		GetOverlappedResult( prx, &ovrx, &bytes, FALSE );

		return QString::fromLocal8Bit( buffer, bytes );
	}

	QByteArray pipe;

	HANDLE ptx, prx;

	OVERLAPPED ovrx;
	char buffer[ buffer_size ];

#else

	pi_State( LuaThread* parent ) :
		controller( parent ),
		thread( 0 )
	{
		socketpair( AF_UNIX, SOCK_RAW, 0, sv );
	}

	~pi_State( void )
	{
		close( sv[0] );
		close( sv[1] );
	}

	QString readrx( void )
	{
		int available;
		ioctl( sv[0], FIONREAD, &available );

		if( available > 0 )
		{
			QByteArray buffer( available, '\0' );
			recv( sv[0], buffer.data(), available, 0 );

			return QString::fromUtf8( buffer );
		}

		return QString();
	}

	int sv[2];

#endif

	LuaThread* controller;
	std::thread* thread;
	QByteArray script;

	// search paths to add to lua vm (for loading packages)
	QStringList searchdirs;

	// setjmp/longjmp for exiting vm
	bool exitflag;
	jmp_buf exitjmp;

	static void lua_hook( lua_State* L, lua_Debug* arg )
	{
		lua_getfield( L, LUA_REGISTRYINDEX, "_control_state" );
		pi_State* state = (pi_State*) lua_touserdata( L, -1 );
		lua_pop( L, 1 );

		QMetaObject::invokeMethod( state->controller, "currentLine", Qt::QueuedConnection, Q_ARG(int,arg->currentline) );

		if( state->exitflag )
		{
			longjmp( state->exitjmp, 1 );
		}
	}
};


LuaThread::LuaThread( QObject* parent ) :
	QObject( parent ),
	m_state( new pi_State( this ) )
{
#ifdef _WIN32
	QWinEventNotifier* pevt = new QWinEventNotifier( this );
	pevt->setHandle( m_state->ovrx.hEvent );
	connect( pevt, SIGNAL(activated(HANDLE)), this, SLOT(pipe_rx()) );
	pevt->setEnabled( true );
#else
	QSocketNotifier* notifier = new QSocketNotifier( m_state->sv[0], QSocketNotifier::Read, this );
	connect( notifier, SIGNAL(activated(int)), this, SLOT(pipe_rx()) );
	notifier->setEnabled( true );
#endif
}


LuaThread::~LuaThread( void )
{
	stop();
	if( ! wait( 10000 ) )
	{
		terminate();
	}

	delete m_state;
}


void LuaThread::pipe_rx( void )
{
	emit fromStdOut( m_state->readrx() );

#ifdef _WIN32
	m_state->restartrx();
#endif
}


void LuaThread::setScript( QString const& text )
{
	m_state->script = text.toUtf8();
}


bool LuaThread::isRunning( void )
{
	if( m_state->thread == 0 )
	{
		return false;
	}

	if( m_state->thread->joinable() )
	{
		return false;
	}

	return true;
}


void LuaThread::start( void )
{
	if( isRunning() )
	{
		return;
	}

	stop();

	if( m_state->thread == 0 )
	{
		m_state->thread = new std::thread( std::bind( &LuaThread::thread, this ) );
	}
}


void LuaThread::stop( void )
{
	if( m_state->thread )
	{
		m_state->exitflag = true;

		if( m_state->thread->joinable() )
		{
			m_state->thread->join();
			delete m_state->thread;
			m_state->thread = 0;
		}
	}
}


bool LuaThread::wait( unsigned s )
{
	return true;
}


void LuaThread::terminate( void )
{
}


void LuaThread::setSearchDirs( QString const& dir )
{
	m_state->searchdirs.clear();
	m_state->searchdirs << dir;
}

void LuaThread::setSearchDirs( QStringList const& dirs )
{
	m_state->searchdirs = dirs;
}


namespace
{
	int luaprint (lua_State *L)
	{
		// number of arguments
		int n = lua_gettop( L );

		// get output file stream
		lua_getfield( L, LUA_REGISTRYINDEX, "_IO_output" );
		luaL_Stream* stream = (luaL_Stream*) lua_touserdata( L, -1 );
		lua_pop( L, 1 );

		// get tostring helper
		lua_getglobal( L, "tostring" );

		for( int i = 1; i <= n; i++ )
		{
			const char *s;
			size_t l;

			lua_pushvalue( L, -1 );			// tostring function
			lua_pushvalue( L, i );			// value
			lua_call(L, 1, 1);				// call tostring
			s = lua_tolstring(L, -1, &l);	// get result

			if( s == NULL )
				return luaL_error(L, "'tostring' must return a string to 'print'");

			if( i > 1 ) fputc( '\t', stream->f );
			fwrite( s, l, 1, stream->f );

			lua_pop(L, 1);					// pop result
		}
		fputc( '\n', stream->f );
		return 0;
	}

	int closefn( lua_State* L )
	{
		(void) L;
		return 0;
	}

	int luatraceback( lua_State* L )
	{
		char const* msg = 0;

		if( lua_isstring( L, lua_gettop( L ) ) )
		{
			msg = lua_tostring( L, lua_gettop( L ) );
		}

		luaL_traceback( L, L, msg, 1 );
		return 1;
	}
}

void LuaThread::thread( void )
{
	QMetaObject::invokeMethod( this, "started", Qt::QueuedConnection );

	lua_State* L = luaL_newstate();
	luaL_openlibs( L );

	//
	// override lua print function (to use io.stdout)
	//

	lua_pushcfunction( L, luaprint );
	lua_setglobal( L, "print" );

	//
	// Set io.stdout file handle
	//

#ifdef _WIN32
	int fd = _open_osfhandle( (intptr_t) m_state->ptx, _O_BINARY | _O_WRONLY );
#else
	int fd = m_state->sv[1];
#endif

	luaL_Stream* stream = (luaL_Stream*) lua_newuserdata( L, sizeof(luaL_Stream) );
	stream->f = fdopen( fd, "w" );
	setvbuf( stream->f, NULL, _IONBF, 0 );
	stream->closef = &closefn;

	luaL_getmetatable( L, LUA_FILEHANDLE );
	lua_setmetatable( L, -2 );

	lua_getglobal( L, "io" );							// io,<file>
	lua_pushvalue( L, -2 );								// <file>,io,<file>
	lua_setfield( L, -2, "stdout" );					// io,<file>
	lua_pop( L, 1 );									// <file>
	lua_pushvalue( L, -1 );								// <file>,<file>
	lua_setfield( L, LUA_REGISTRYINDEX, "_IO_output" );	// <file>
	lua_pop( L, 1 );									// empty!

	//
	// append script search dirs to package.path
	//

	lua_getglobal( L, "package" );
	lua_pushstring( L, "" );
	for( auto const& dir : m_state->searchdirs )
	{
		lua_pushstring( L, dir.toUtf8().data() );
		lua_pushstring( L, "/?.lua" );
		lua_pushstring( L, ";" );
		lua_concat( L, 4 );
	}
#ifdef _WIN32
	luaL_gsub( L, lua_tostring( L, -1 ), "/", LUA_DIRSEP );
	lua_replace( L, -2 );
#endif
	lua_getfield( L, -2, "path" );
	lua_concat( L, 2 );
	lua_setfield( L, -2, "path" );
	lua_pop( L, 1 );

	// push state pointer into registry
	lua_pushlightuserdata( L, m_state );
	lua_setfield( L, LUA_REGISTRYINDEX, "_control_state" );

	// set flags, hooks...
	m_state->exitflag = false;
	lua_sethook( L, &pi_State::lua_hook, LUA_MASKLINE, 0 );

	// exit point for stop event
	if( setjmp( m_state->exitjmp ) == 0 )
	{
		// backtrace maker
		lua_pushcclosure( L, luatraceback, 0 );

		int err = luaL_loadbuffer( L, m_state->script.data(), m_state->script.length(), "=script" );
		if( err == LUA_OK )
		{
			err = lua_pcall( L, 0, LUA_MULTRET, -2 );
		}

		if( err == LUA_ERRRUN || err == LUA_ERRSYNTAX )
		{
			char const* str;
			size_t n;
			str = lua_tolstring( L, -1, &n );
			fwrite( str, n, 1, stream->f );
			fputc( '\n', stream->f );
		}
	}

	lua_close( L );

	QMetaObject::invokeMethod( this, "stopped", Qt::QueuedConnection );
}
