#pragma once
#include "EventThread.h"
#include <afxmt.h>
#include "sqlite/CppSQLite3.h"

class CQPasteWndThread: public CEventThread
{
public:
    CQPasteWndThread(void);
    ~CQPasteWndThread(void);

    enum eCQPasteWndThreadEvents
    {
		DO_SET_LIST_COUNT, 
		LOAD_ACCELERATORS, 
		UNLOAD_ACCELERATORS, 
		LOAD_ITEMS, 
		LOAD_EXTRA_DATA, 

        ECQPASTEWNDTHREADEVENTS_COUNT  //must be last

    };

    void FireSetListCount()
    {
        FireEvent(DO_SET_LIST_COUNT);
    }
    void FireLoadItems(bool firstLoad)
    {
        FireEvent(LOAD_ITEMS);
    }
    void FireLoadExtraData(int rowHeight)
    {
		m_rowHeight = rowHeight;
        FireEvent(LOAD_EXTRA_DATA);
    }
    void FireLoadAccelerators()
    {
        FireEvent(LOAD_ACCELERATORS);
    }
    void FireUnloadAccelerators()
    {
        FireEvent(UNLOAD_ACCELERATORS);
    }

    HANDLE m_SearchingEvent;

	void SetRowHeight(int height) { m_rowHeight = height; }
    void SetSearchSql(CString sql, CString countSql, CString baseFilter = _T(""), CString countPrefix = _T(""))
    {
        ATL::CCritSecLock csLock(m_searchLock.m_sect);
        ++m_searchGeneration;
        m_sql = sql;
        m_countSql = countSql;
        m_baseFilter = baseFilter;
        m_countPrefix = countPrefix;
    }

    bool IsCurrentSearch(UINT_PTR generation)
    {
        ATL::CCritSecLock csLock(m_searchLock.m_sect);
        return generation == m_searchGeneration;
    }

protected:
    virtual void OnEvent(int eventId, void *param);
    virtual void OnTimeOut(void *param);

    void OnSetListCount(void *param);
    void OnLoadItems(void *param);
    void OnLoadExtraData(void *param);
	//void ReduceMapItems(CF_DibTypeMap mapItem, CCriticalSection &critSection, CString mapName);
    void OnLoadAccelerators(void *param);
    void OnUnloadAccelerators(void *param);

	CString EnumName(eCQPasteWndThreadEvents e);

	int m_rowHeight;

    CCriticalSection m_searchLock;
    UINT_PTR m_searchGeneration = 0;
    CString m_sql;
    CString m_countSql;
    CString m_baseFilter;
    CString m_countPrefix;
};
