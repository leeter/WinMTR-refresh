#include "WinMTRGlobal.h"
#include "WinMTRStatusBar.h"
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(WinMTRStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(WinMTRStatusBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// WinMTRStatusBar message handlers
//////////////////////////////////////////////////////////////////////////

int WinMTRStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if( CStatusBar::OnCreate(lpCreateStruct) == -1 )
		return -1;
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////

LRESULT WinMTRStatusBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult =CStatusBar::WindowProc(message, wParam, lParam);
	if( message == WM_SIZE ){
		RepositionControls();
	}
	return lResult;
}

//////////////////////////////////////////////////////////////////////////

void WinMTRStatusBar::RepositionControls()
{
	HDWP _hDWP = ::BeginDeferWindowPos( gsl::narrow_cast<int>(m_arrPaneControls.size()) );
	
	CRect rcClient;
	GetClientRect(&rcClient);
	for (const auto & cntl : m_arrPaneControls)
	{
		int   iIndex  = CommandToIndex(cntl.nID);
		HWND hWnd    = cntl.hWnd;
		
		CRect rcPane;
		GetItemRect(iIndex, &rcPane);
		
		// CStatusBar::GetItemRect() sometimes returns invalid size 
		// of the last pane - we will re-compute it
		int cx = ::GetSystemMetrics( SM_CXEDGE );
		DWORD dwPaneStyle = GetPaneStyle( iIndex );
		if( iIndex == (m_nCount-1) )
		{
			if( (dwPaneStyle & SBPS_STRETCH ) == 0 )
			{
				UINT nID, nStyle;
				int  cxWidth;
				GetPaneInfo( iIndex, nID, nStyle, cxWidth );
				rcPane.right = rcPane.left + cxWidth + cx*3;
			} // if( (dwPaneStyle & SBPS_STRETCH ) == 0 )
			else
			{
				CRect rcClient;
				GetClientRect( &rcClient );
				rcPane.right = rcClient.right;
				if( (GetStyle() & SBARS_SIZEGRIP) == SBARS_SIZEGRIP )
				{
					int cxSmIcon = ::GetSystemMetrics( SM_CXSMICON );
					rcPane.right -= cxSmIcon + cx;
				} // if( (GetStyle() & SBARS_SIZEGRIP) == SBARS_SIZEGRIP )
			} // else from if( (dwPaneStyle & SBPS_STRETCH ) == 0 )
		} // if( iIndex == (m_nCount-1) )
		
		if ((GetPaneStyle (iIndex) & SBPS_NOBORDERS) == 0){
			rcPane.DeflateRect(cx,cx);
		}else{
			rcPane.DeflateRect(cx,1,cx,1);
		}
		
		if (hWnd && ::IsWindow(hWnd)){
			_hDWP = ::DeferWindowPos(
				_hDWP, 
				hWnd, 
				NULL, 
				rcPane.left,
				rcPane.top, 
				rcPane.Width(), 
				rcPane.Height(),
				SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_SHOWWINDOW
				);

			::RedrawWindow(
				hWnd,
				NULL,
				NULL,
				RDW_INVALIDATE|RDW_UPDATENOW
				|RDW_ERASE|RDW_ERASENOW
				);
			
		} // if (hWnd && ::IsWindow(hWnd)){ 
	}
	
	VERIFY( ::EndDeferWindowPos( _hDWP ) );
};

//////////////////////////////////////////////////////////////////////////

BOOL WinMTRStatusBar::AddPane(
	 UINT nID,	// ID of the  pane
	 int nIndex	// index of the pane
	 )
{
	if (nIndex < 0 || nIndex > m_nCount){
		ASSERT(FALSE);
		return FALSE;
	}
	
	if (CommandToIndex(nID) != -1){
		ASSERT(FALSE);
		return FALSE;
	}
	
	std::vector<_STATUSBAR_PANE_> arrPanesTmp;
	arrPanesTmp.reserve(static_cast<size_t>(m_nCount) + 1);
	for (int iIndex = 0; iIndex < m_nCount+1; iIndex++)
	{
		_STATUSBAR_PANE_ pNewPane;
		
		if (iIndex == nIndex){
			pNewPane.nID    = nID;
			pNewPane.nStyle = SBPS_NORMAL;
		}else{
			int idx = iIndex;
			if (iIndex > nIndex) idx--;
			
			_STATUSBAR_PANE_* pOldPane  = GetPanePtr(idx);
			pNewPane = *pOldPane;
		}
		arrPanesTmp.emplace_back(std::move(pNewPane));
	}

	std::vector<UINT> IDArray;
	IDArray.reserve(arrPanesTmp.size());
	for (const auto& pane : arrPanesTmp) {
		IDArray.emplace_back(pane.nID);
	}
	
	// set the indicators 
	SetIndicators(IDArray.data(), gsl::narrow_cast<int>(IDArray.size()));
	// free memory
	auto nPanesCount = arrPanesTmp.size();
	for (size_t iIndex = 0; iIndex < nPanesCount; iIndex++){
		if (iIndex != nIndex)
			PaneInfoSet(iIndex, arrPanesTmp[iIndex]);
	}
	
	
	RepositionControls();
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

BOOL WinMTRStatusBar::RemovePane(
	UINT nID	// ID of the pane
	)
{
	if ( CommandToIndex(nID) == -1 || m_nCount == 1 ){
		ASSERT(FALSE);
		return FALSE;
	}
	
	std::vector<_STATUSBAR_PANE_> arrPanesTmp;
	arrPanesTmp.reserve(m_nCount);
	for (int nIndex = 0; nIndex < m_nCount; nIndex++)
	{
		_STATUSBAR_PANE_* pOldPane = GetPanePtr(nIndex);
		
		if (pOldPane->nID == nID)
			continue;
		
		_STATUSBAR_PANE_ pNewPane = *pOldPane;
		arrPanesTmp.emplace_back(std::move(pNewPane));
	}
	std::vector<UINT> IDArray;
	IDArray.reserve(arrPanesTmp.size());
	for (const auto &pane : arrPanesTmp) {
		IDArray.emplace_back(pane.nID);
	}
	
	// set the indicators
	SetIndicators(IDArray.data(), IDArray.size());
	// free memory
	int nIndex = 0;
	for (const auto& pane : arrPanesTmp) {
		this->PaneInfoSet(nIndex, pane);
		++nIndex;
	}
	m_arrPaneControls.erase(std::remove_if(m_arrPaneControls.begin(), m_arrPaneControls.end(), [nID](const auto& cntl) {
		return cntl.nID == nID;
		}),
		m_arrPaneControls.end());
	
	RepositionControls();
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

BOOL WinMTRStatusBar::AddPaneControl(HWND hWnd, UINT nID, BOOL bAutoDestroy)
{
	if (CommandToIndex (nID) == -1) {
		return FALSE;
	}
	
	m_arrPaneControls.emplace_back(hWnd, nID, bAutoDestroy);

	RepositionControls();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

bool WinMTRStatusBar::PaneInfoGet(int nIndex, _STATUSBAR_PANE_* pPane)
{
	if( nIndex < m_nCount  && nIndex >= 0 )
	{
		GetPaneInfo( nIndex,  pPane->nID, pPane->nStyle, pPane->cxText );
		CString strPaneText;
		GetPaneText( nIndex , strPaneText );
		pPane->strText = LPCTSTR(strPaneText);
		return true;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool WinMTRStatusBar::PaneInfoSet(int nIndex, const _STATUSBAR_PANE_& pPane)
{
	if( nIndex < m_nCount  && nIndex >= 0 ){
		SetPaneInfo( nIndex, pPane.nID, pPane.nStyle, pPane.cxText );
		SetPaneText( nIndex, static_cast<LPCTSTR>( pPane.strText) );
		return true;
	}
	return false;
}

WinMTRStatusBar::_STATUSBAR_PANE_CTRL_::_STATUSBAR_PANE_CTRL_()
	:hWnd(nullptr),
	nID(0),
	bAutoDestroy(false)
{
}

WinMTRStatusBar::_STATUSBAR_PANE_CTRL_::~_STATUSBAR_PANE_CTRL_()
{
	if (this->hWnd && ::IsWindow(this->hWnd)) {
		::ShowWindow(this->hWnd, SW_HIDE);
		if (this->bAutoDestroy) {
			::DestroyWindow(this->hWnd);
		}
	}
}
