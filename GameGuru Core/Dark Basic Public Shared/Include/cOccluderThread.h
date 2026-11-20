#include "cThread.h"
#include "SoftwareCulling.h"

extern bool g_bEarlyExcludeMode;
extern void DoEarlyExclude ( void );

class cOccluderThread : public cThread
{
	public:
		cOccluderThread ( )
		{
		}

		~cOccluderThread ( )
		{
		}

		unsigned int Run ( )
		{
			// PW Edition: the worker must stop before its level's objects are freed.
			// game_freelevel() calls Stop(), wakes the begin event, and Join().
			while ( !m_bTerminate )
			{
				if ( !g_hOccluderBegin ) break;
				const DWORD status = WaitForSingleObject ( g_hOccluderBegin, INFINITE );
				if ( status != WAIT_OBJECT_0 || m_bTerminate ) break;
				CPU3DDoOcclude ();
			}
			if ( g_hOccluderEnd ) SetEvent ( g_hOccluderEnd );

			return 1;
		}
};
