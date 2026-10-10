#include "stdafx.h"
#include "QPasteWndThread.h"
#include "Misc.h"
#include "Options.h"
#include "QPasteWnd.h"
#include "cp_main.h"
#include <vector>
#include <algorithm>

CQPasteWndThread::CQPasteWndThread(void)
{
	m_rowHeight = 0;
	m_threadName = "CQPasteWndThread";
    m_waitTimeout = ONE_HOUR * 12;

    m_SearchingEvent = CreateEvent(NULL, TRUE, FALSE, _T(""));

    for(int eventEnum = 0; eventEnum < ECQPASTEWNDTHREADEVENTS_COUNT; eventEnum++)
    {
        AddEvent(eventEnum);
    }
}

CQPasteWndThread::~CQPasteWndThread(void)
{
    CloseHandle(m_SearchingEvent);
}

void CQPasteWndThread::OnTimeOut(void *param)
{
}

void CQPasteWndThread::OnEvent(int eventId, void *param)
{
	DWORD startTick = GetTickCount();
	Log(StrF(_T("Start of OnEvent, eventId: %s"), EnumName((eCQPasteWndThreadEvents)eventId)));

    switch((eCQPasteWndThreadEvents)eventId)
    {
        case DO_SET_LIST_COUNT:
            OnSetListCount(param);
            break;
        case LOAD_ACCELERATORS:
            OnLoadAccelerators(param);
            break;
        case UNLOAD_ACCELERATORS:
            OnUnloadAccelerators(param);
            break;
        case LOAD_ITEMS:
            OnLoadItems(param);
            break;
        case LOAD_EXTRA_DATA:
            OnLoadExtraData(param);
            break;
    }

	DWORD length = GetTickCount() - startTick;
	Log(StrF(_T("End of OnEvent, eventId: %s, Time: %d(ms)"), EnumName((eCQPasteWndThreadEvents)eventId), length));
}

// THREAD-BOUNDARY FOLLOW-UPS (deliberately NOT fixed in this change).
// Ad-hoc UI-thread DB hits found by inspection; each blocks the message
// pump while SQLite runs. Left for later tasks -- see the report.
//   - CQPasteWnd::OnMenuGoToEntry (QPasteWnd.cpp ~7209/7252/7270): key fetch
//     + ordered rank query + COUNT-fallback COUNT all run on the UI thread,
//     then FillList + a 5s message-pumping wait on m_SearchingEvent.
//   - Tooltip/status single-row SELECT (~1321) and clip-property SELECTs
//     (~4320, ~6057): PK lookups, cheap but synchronous on the UI thread.
//   - ~15 CClip::LoadMainTable/LoadFormats call sites on paste/property
//     paths: synchronous by necessity (need the bytes to paste); batching
//     opportunities exist but change paste semantics -- separate task.
//   - OnLoadAccelerators/OnUnloadAccelerators refresh the list accelerators
//     (UI CWnd state: m_Accels + registry reads) from the worker thread.
//     Pre-existing; needs a UI-thread marshal, not done here.
//     Pre-existing; needs a UI-thread marshal, not done here.
//   - SetSearchSql writes m_sql/m_countSql with no lock while the worker
//     copies them (same as before this change); do not add string-compare
//     staleness guards without first putting the writer under m_CritSection.
//   - HideQPasteWindow/OnRefreshView clear m_listItems + SetItemCountEx(0)
//     after waiting on m_SearchingEvent WITHOUT pumping messages, so an
//     already-queued NM_SET_LIST_COUNT is still dispatched afterwards and
//     sets a stale count on an empty list. Pre-existing window, unchanged
//     by making COUNT async (same single-sender FIFO as before).
void CQPasteWndThread::OnSetListCount(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    static CEvent UpdateTimeEvent(TRUE, TRUE, _T("Ditto_Update_Clip_Time"), NULL);
    //If we pasted then wait for the time on the pasted event to be updated before we query the db
    DWORD dRet = WaitForSingleObject(UpdateTimeEvent, 2000);

    ResetEvent(m_SearchingEvent);
    long lTick = GetTickCount();

	CString countSQL = m_countSql;

    long lRecordCount = 0;

    // Set false only on the stale-drop path that has a newer LOAD queued
    // behind this COUNT (see below). Declared before try so the exception
    // path still signals -- a failed COUNT must not leave waiters stranded.
    bool signalCompletion = true;

    try
    {
        lRecordCount = theApp.m_db.execScalar(countSQL);
        // Stale-result guard (existing protocol only, no new state): if a
        // newer FillList/Hide set m_bStopQuery after this COUNT was queued,
        // this count belongs to an older search -- drop the post. Both sides
        // use pasteWnd->m_CritSection, so this read is synchronized with the
        // writers. Dropped is always safe: the newer search queues its own
        // COUNT behind its page load. If the newer LOAD already reset the
        // flag, we may still post a stale count, but single-sender PostMessage
        // FIFO guarantees the newer COUNT lands after it -- the UI converges.
        //
        // Completion signaling is tied to the current generation: when stale
        // AND a newer LOAD is still queued (m_loadItems non-empty), skip the
        // trailing SetEvent so a waiter (GoToEntry's pump-wait) is not
        // released on this stale generation with zero/provisional rows --
        // the queued LOAD -> COUNT handoff keeps the event reset across and
        // signals on the generation the waiter actually waits for. When stale
        // with an empty queue, the stop came from HideQPasteWindow (1016) /
        // OnRefreshView (1443), which wait on this event WITHOUT queueing a
        // newer generation, so still SetEvent to release them promptly
        // instead of forcing their 5s timeout. Single worker thread means a
        // queued LOAD cannot run concurrently with this COUNT, so an empty
        // queue provably means no newer generation is owed a signal, and
        // every skip is covered by the later generation's Set.
        bool stale = false;
        bool newerLoadPending = false;
        {
            ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);
            stale = pasteWnd->m_bStopQuery;
            if (stale)
            {
                newerLoadPending = !pasteWnd->m_loadItems.empty();
            }
        }
        if (stale)
        {
            if (newerLoadPending)
            {
                Log(_T("Dropping stale list count without signaling, newer load pending"));
                signalCompletion = false;
            }
            else
            {
                Log(_T("Dropping stale list count, no newer load (hide/cancel) - signaling"));
            }
        }
        else
        {
            ::PostMessage(pasteWnd->m_hWnd, NM_SET_LIST_COUNT, lRecordCount, 0);
        }
    }
    CATCH_SQLITE_EXCEPTION 

    if (signalCompletion)
    {
        SetEvent(m_SearchingEvent);
    }

    Log(StrF(_T("Set list count = %d, time = %d"), lRecordCount, GetTickCount() - lTick));
}

void CQPasteWndThread::OnLoadItems(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    ResetEvent(m_SearchingEvent);

    // Set when this invocation queues COUNT async (first load below); the
    // queued COUNT handler then owns the trailing SetEvent so the event
    // stays reset across the LOAD -> COUNT handoff.
    bool firedAsyncCount = false;

	while(true)
	{
		long startTick = GetTickCount();
	    int loadItemsIndex = 0;
	    int loadItemsCount = 0;
	    int loadCount = 0;
		CString localSql = m_sql;
	    bool clearFirstLoadItem = false;
		bool firstLoad = false;
		int listSize = 0;
		{
			ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		    if(pasteWnd->m_loadItems.size() > 0)
		    {
				firstLoad = (pasteWnd->m_loadItems.begin()->x == -1);
		        loadItemsIndex = max(pasteWnd->m_loadItems.begin()->x, 0);
		        loadItemsCount = pasteWnd->m_loadItems.begin()->y - pasteWnd->m_loadItems.begin()->x;
		        pasteWnd->m_bStopQuery = false;
				listSize = pasteWnd->m_listItems.size();
		        clearFirstLoadItem = true;
		    }
		}

	    if(clearFirstLoadItem)
	    {
			try
			{
				Log(StrF(_T("Load Items start = %d, count = %d, list size: %d"), loadItemsIndex, loadItemsCount, listSize));

				int pos = loadItemsIndex;
				CString limit;
				limit.Format(_T(" LIMIT %d OFFSET %d"), loadItemsCount, loadItemsIndex);
				localSql += limit;

				CMainTable table;

				CppSQLite3Query q = theApp.m_db.execQuery(localSql);
				while(!q.eof())
				{
					CQPasteWnd::FillMainTable(table, q);

					int updateIndex = -1;

					{
						ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

						if (pos < pasteWnd->m_listItems.size())
						{
							pasteWnd->m_listItems[pos] = table;

							updateIndex = pos;

							//Log(StrF(_T("updating list pos = %d, id: %d, size: %d"), pos, table.m_lID, pasteWnd->m_listItems.size() - 1));
						}
						else if (pos == pasteWnd->m_listItems.size())
						{
							pasteWnd->m_listItems.push_back(table);
							updateIndex = (int)pasteWnd->m_listItems.size() - 1;
							//Log(StrF(_T("adding (same size) list pos = %d, id: %d, size: %d"), pasteWnd->m_listItems.size()-1, table.m_lID, pasteWnd->m_listItems.size() - 1));
						}
						else if (pos > pasteWnd->m_listItems.size())
						{
							for (int toAdd = (int)pasteWnd->m_listItems.size()-1; toAdd < pos - 1; toAdd++)
							{
								CMainTable empty;
								empty.m_lID = -1;
								pasteWnd->m_listItems.push_back(empty);

								//Log(StrF(_T("adding dummy row size: %d"), pasteWnd->m_listItems.size()-1));
							}

							pasteWnd->m_listItems.push_back(table);

							updateIndex = (int)pasteWnd->m_listItems.size() - 1;

							//Log(StrF(_T("adding list pos = %d, id: %d, size: %d"), pasteWnd->m_listItems.size()-1, table.m_lID, pasteWnd->m_listItems.size() - 1));
						}
					}

					if(pasteWnd->m_bStopQuery)
					{
						Log(StrF(_T("StopQuery called exiting filling cache count = %d"), loadItemsIndex));
						break;
					}

					q.nextRow();

					if(firstLoad == false)
					{
						/*if (updateIndex != loadItemsIndex)
						{
							Log(StrF(_T("index difference old: %d, new: %d"), loadItemsIndex, updateIndex));
						}*/

	            		::PostMessage(pasteWnd->m_hWnd, NM_REFRESH_ROW, table.m_lID, updateIndex);
					}

					loadItemsIndex++;
					loadCount++;
					pos++;
				}

			DWORD loadCount = GetTickCount() - startTick;
			DWORD countCount = 0;
			DWORD acceleratorCount = 0;

			if(firstLoad)
			{
				// First-load COUNT is non-blocking: the page window above is
				// already committed to m_listItems under m_CritSection, so
				// post its -2 repaint first and queue COUNT + accelerators as
				// later events on this same thread (existing auto-reset
				// signals via FireEvent -- no new threads, locks, or message
				// types). The UI exposes the provisional page in
				// OnRefeshRow(-2) and reconciles to the full count when
				// NM_SET_LIST_COUNT lands. Ordering guarantee: both posts
				// originate on this thread in -2-then-COUNT order and
				// PostMessage preserves per-sender FIFO, so for one search
				// the visible count only grows 0 -> page -> full; a newer
				// search resets to 0 in FillList before its own -2/COUNT pair.
				::PostMessage(pasteWnd->m_hWnd, NM_REFRESH_ROW, -2, 0);

				FireSetListCount();
				FireLoadAccelerators();
				firedAsyncCount = true;

				// COUNT/accel durations are logged in their own handlers now;
				// here we keep only the page-fill time in the End line below.
				countCount = 0;
				acceleratorCount = 0;
			}
				else
				{
					::PostMessage(pasteWnd->m_hWnd, NM_REFRESH_ROW, -1, 0);
				}

				if(clearFirstLoadItem)
				{
					ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

					pasteWnd->m_loadItems.erase(pasteWnd->m_loadItems.begin());
				}

				Log(StrF(_T("Load items End count = %d, Total Time = %d, LoadItems: %d, Count: %d, Accel: %d"), loadCount, GetTickCount() - startTick, loadCount, countCount, acceleratorCount));
			}
			catch (CppSQLite3Exception& e)	\
			{								\
				Log(StrF(_T("ONLoadItems - SQLITE Exception %d - %s"), e.errorCode(), e.errorMessage()));	\
				ASSERT(FALSE);				\
				break;
			}	
		}
		else
		{
			break;
		}
	}

    // If first-load queued COUNT async, its handler Reset/Sets the event
    // around itself, so skip the Set here: the event stays reset across the
    // LOAD -> COUNT handoff and waiters (GoToEntry, HideQPasteWindow) cannot
    // wake on the provisional page and mistake it for a complete search.
    // No deadlock: every waiter uses a timeout, and the queued COUNT always
    // runs next unless the thread is stopped (whose waiters time out too).
    if (firedAsyncCount == false)
    {
        SetEvent(m_SearchingEvent);
    }
}

void ReduceMapItems(CF_DibTypeMap &mapItem, CCriticalSection &critSection, CString mapName)
{
	ATL::CCritSecLock csLock(critSection.m_sect);

	int maxSize = 50;
	int reduceToSize = 30;

	if (mapItem.size() > maxSize)
	{
		//create a vector so we can sort and keep the last x number of events
		vector<INT64> counterArray;
		for (CF_DibTypeMap::iterator iterDib = mapItem.begin(); iterDib != mapItem.end(); iterDib++)
		{
			counterArray.push_back(iterDib->second.m_counter);
		}
		std::sort(counterArray.begin(), counterArray.end());
		counterArray.erase(counterArray.begin(), counterArray.end() - reduceToSize);

		//remove the oldest x number if bitmaps
		for (CF_DibTypeMap::iterator iterDib = mapItem.begin(); iterDib != mapItem.end();)
		{
			if (std::binary_search(counterArray.begin(), counterArray.end(), iterDib->second.m_counter) == false)
			{
				Log(StrF(_T("reduced size of %s cache, Id: %d, Row: %d"), mapName, iterDib->second.m_parentId, iterDib->second.m_clipRow));

				mapItem.erase(iterDib++);
			}
			else
			{
				++iterDib;
			}
		}

		Log(StrF(_T("reduced size of %s cache, count: %d"), mapName, mapItem.size()));
	}
}

// NOTE (b, SKIPPED): coalescing the per-row theApp.GetClipData loop below
// into one windowed IN-query fetch was judged NOT obviously safe, so the loop
// shape is unchanged. Reasons: per-row failures are contained today (GetClipData
// catches internally per call, plus a CF_DIB->PNG fallback second query), while
// a batched cursor would need a new outer try/catch -- a new crash vector
// through CEventThread::RunThread, which has no handler. The loop also relies
// on per-item m_clipRow freshness, per-type NO-caches, and interleaved GDI
// scaling (GetDibFittingToHeight) whose redistribution is not behavior-neutral.
// Follow-up: batch only the Data fetch for visible rows behind the same
// per-row fallback/cache insertion, measured against a large DB first.
void CQPasteWndThread::OnLoadExtraData(void *param)
{
    ResetEvent(m_SearchingEvent);

    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    Log(_T("Start of load extra data, Bitmaps/rtf"));

    std::list<CClipFormatQListCtrl> localFormats;
	{
		ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		for (std::list<CClipFormatQListCtrl>::iterator it = pasteWnd->m_ExtraDataLoadItems.begin(); it != pasteWnd->m_ExtraDataLoadItems.end(); it++)
		{
			localFormats.push_back(*it);
		}
	    pasteWnd->m_ExtraDataLoadItems.clear();
	}
	
	for (std::list<CClipFormatQListCtrl>::iterator it = localFormats.begin(); it != localFormats.end(); it++)
    {
		bool loadClip = true;

		if (it->m_cfType == CF_DIB)
		{
			ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

			CF_DibTypeMap::iterator iterDib = pasteWnd->m_cf_dibCache.find(it->m_parentId);
			if (iterDib != pasteWnd->m_cf_dibCache.end())
			{
				loadClip = false;
			}
			else
			{
				CF_NoDibTypeMap::iterator iterNoDib = pasteWnd->m_cf_NO_dibCache.find(it->m_parentId);
				if (iterNoDib != pasteWnd->m_cf_NO_dibCache.end())
				{
					loadClip = false;
				}
			}
		}
		else if (it->m_cfType == theApp.m_RTFFormat)
		{
			ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

			CF_DibTypeMap::iterator iterDib = pasteWnd->m_cf_rtfCache.find(it->m_parentId);
			if (iterDib != pasteWnd->m_cf_rtfCache.end())
			{
				loadClip = false;
			}
			else
			{
				CF_NoDibTypeMap::iterator iterNoRtf = pasteWnd->m_cf_NO_rtfCache.find(it->m_parentId);
				if (iterNoRtf != pasteWnd->m_cf_NO_rtfCache.end())
				{
					loadClip = false;
				}
			}
		}

		if (loadClip)
		{
			DWORD startLoadClipData = GetTickCount();

			BOOL foundClipData = theApp.GetClipData(it->m_parentId, *it);
			if (foundClipData == false &&
				it->m_cfType == CF_DIB)
			{
				it->Free();
				it->m_cfType = theApp.m_PNG_Format;

				foundClipData = theApp.GetClipData(it->m_parentId, *it);
			}

			if (foundClipData)
			{
				DWORD timeTook = GetTickCount() - startLoadClipData;
				if (timeTook > 20)
				{
					Log(StrF(_T("GetClipData for clip %d, took: %d"), it->m_parentId, timeTook));
				}

				if (it->m_cfType == CF_DIB ||
					it->m_cfType == theApp.m_PNG_Format)
				{
					DWORD startConvertImage = GetTickCount();

					HDC dc = GetDC(NULL);

					it->GetDibFittingToHeight(CDC::FromHandle(dc), m_rowHeight);

					ReleaseDC(NULL, dc);

					DWORD timeTook = GetTickCount() - startConvertImage;
					if (timeTook > 20)
					{
						Log(StrF(_T("GetDibFittingToHeight for clip %d, took: %d"), it->m_parentId, GetTickCount() - startConvertImage));
					}

					{
						ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

						pasteWnd->m_cf_dibCache[it->m_parentId] = *it;
						//the cache now owns the format data, set it to delete the data in the destructor
						pasteWnd->m_cf_dibCache[it->m_parentId].m_autoDeleteData = true;

						Log(StrF(_T("Loaded, extra data for clipId: %d, Row: %d image cache count: %d"), it->m_parentId, it->m_clipRow, pasteWnd->m_cf_dibCache.size()));
					}
				}
				else if (it->m_cfType == theApp.m_RTFFormat)
				{
					ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

					pasteWnd->m_cf_rtfCache[it->m_parentId] = *it;
					it->m_autoDeleteData = false;
					//the cache now owns the format data, set it to delete the data in the destructor
					pasteWnd->m_cf_rtfCache[it->m_parentId].m_autoDeleteData = true;

					Log(StrF(_T("Loaded, extra data for clip %d, rtf cache count: %d"), it->m_parentId, pasteWnd->m_cf_rtfCache.size()));
				}

				::PostMessage(pasteWnd->m_hWnd, NM_REFRESH_ROW, it->m_parentId, it->m_clipRow);
			}
			else
			{
				ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

				if (it->m_cfType == CF_DIB ||
					it->m_cfType == theApp.m_PNG_Format)
				{
					pasteWnd->m_cf_NO_dibCache[it->m_parentId] = true;
				}
				else if (it->m_cfType == theApp.m_RTFFormat)
				{
					pasteWnd->m_cf_NO_rtfCache[it->m_parentId] = true;
				}
			}
		}

		if (it->m_cfType == CF_DIB)
		{
			ReduceMapItems(pasteWnd->m_cf_dibCache, pasteWnd->m_CritSection, _T("image"));
		}
		else if (it->m_cfType == theApp.m_RTFFormat)
		{
			ReduceMapItems(pasteWnd->m_cf_rtfCache, pasteWnd->m_CritSection, _T("rtf"));
		}
    }

    SetEvent(m_SearchingEvent);
    Log(_T("End of load extra data, Bitmaps/rtf"));
}

// NOTE (c, SKIPPED): merging the two accelerator SELECTs
// (LoadAcceleratorKeys: lShortCut > 0, MoveToGroupShortCut > 0) into the main
// paged pass was judged NOT obviously safe, so this stays a separate queued
// event. Reasons: the counts/rows use different filters (accels are global,
// not search-scoped -- reloading them per search is wasteful but harmless),
// folding them into the page SQL would change SQL shapes (out of scope), and
// running them synchronously inside the page fill would re-serialize global
// hotkey loading onto first paint -- the opposite of (a). Follow-up: skip
// the reload when no shortcut-bearing row changed, or move it off first load.
void CQPasteWndThread::OnLoadAccelerators(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;
    pasteWnd->RefreshListAccelerators(TRUE);
}

void CQPasteWndThread::OnUnloadAccelerators(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;
    pasteWnd->RefreshListAccelerators(FALSE);
}

CString CQPasteWndThread::EnumName(eCQPasteWndThreadEvents e)
{
	switch(e)
	{
	case DO_SET_LIST_COUNT:
		return _T("Load List Count");
	case LOAD_ACCELERATORS:
		return _T("Load Accelerators");
	case UNLOAD_ACCELERATORS:
		return _T("Unload Accelerators");
	case LOAD_ITEMS:
		return _T("Load clips");
	case LOAD_EXTRA_DATA:
		return _T("Load Extra Data (rtf/bitmaps)");
	}

	return _T("");
}