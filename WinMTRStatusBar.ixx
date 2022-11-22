/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2021 Leetsoftwerx

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//#ifndef WINMTRSTATUSBAR_H_
//#define WINMTRSTATUSBAR_H_
module;
#include "targetver.h"
#include <afxext.h>
export module WinMTRStatusBar;
#pragma warning (disable : 4005)
import <vector>;
import <span>;

export class WinMTRStatusBar final : public CStatusBar
{
// Construction
public:

// Attributes
public:

	// Operations
public:
	
	int GetPanesCount() const noexcept{
		return m_nCount;
	}
	
	void SetPaneWidth(int nIndex, int nWidth)
	{
		_STATUSBAR_PANE_ pane;
		PaneInfoGet(nIndex, &pane);
		pane.cxText = nWidth;
		PaneInfoSet(nIndex, pane);
	}
	
	BOOL AddPane(
		UINT nID,	// ID of the  pane
		int nIndex	// index of the pane
		);

	BOOL RemovePane(
		UINT nID	// ID of the pane
		);

	BOOL AddPaneControl(CWnd* pWnd, UINT nID, BOOL bAutoDestroy)
	{
		return AddPaneControl( pWnd->GetSafeHwnd(), nID, bAutoDestroy);
	}
	
	BOOL AddPaneControl(HWND hWnd, UINT nID, BOOL bAutoDestroy);
	
	void DisableControl( int nIndex, BOOL bDisable=TRUE) noexcept
	{
		const UINT uItemID = GetItemID(nIndex);
		for (const auto & cntl : m_arrPaneControls){
			if( uItemID == cntl.nID ){
				if(cntl.hWnd && ::IsWindow(cntl.hWnd) ) {
					::EnableWindow(cntl.hWnd, bDisable);
				}
			}
		}
	}

	void SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth) noexcept
	{
		CStatusBar::SetPaneInfo(nIndex, nID, nStyle, cxWidth);
		BOOL bDisabled = ((nStyle&SBPS_DISABLED) == 0);
		DisableControl(nIndex, bDisabled);
	}

	void SetPaneStyle(int nIndex, UINT nStyle) noexcept
	{
		CStatusBar::SetPaneStyle(nIndex, nStyle);
		BOOL bDisabled = ((nStyle&SBPS_DISABLED) == 0);
		DisableControl(nIndex, bDisabled);
	}
	
	BOOL SetIndicators(std::span<UINT> indicators) noexcept {
		return CStatusBar::SetIndicators(indicators.data(), static_cast<int>(indicators.size()));
	}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WinMTRStatusBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~WinMTRStatusBar() = default;

protected:

	struct _STATUSBAR_PANE_
	{
		_STATUSBAR_PANE_() noexcept{
			nID = cxText = nStyle = nFlags = 0;
		}
		
		UINT    nID;        // IDC of indicator: 0 => normal text area
		int     cxText;     // width of string area in pixels
		//   on both sides there is a 3 pixel gap and
		//   a one pixel border, making a pane 6 pixels wider
		UINT    nStyle;     // style flags (SBPS_*)
		UINT    nFlags;     // state flags (SBPF_*)
		CString strText;    // text in the pane
	};
	
	struct _STATUSBAR_PANE_CTRL_
	{
		HWND hWnd;
		UINT nID;
		bool bAutoDestroy;
		_STATUSBAR_PANE_CTRL_(HWND hWnd, UINT nID, bool bAutoDestroy) noexcept
			:hWnd(hWnd),nID(nID),bAutoDestroy(bAutoDestroy){}
		_STATUSBAR_PANE_CTRL_() noexcept
			:_STATUSBAR_PANE_CTRL_(nullptr, 0, false){}
		_STATUSBAR_PANE_CTRL_(const _STATUSBAR_PANE_CTRL_&) = delete;
		~_STATUSBAR_PANE_CTRL_() noexcept;
		_STATUSBAR_PANE_CTRL_(_STATUSBAR_PANE_CTRL_&&) noexcept = default;
		_STATUSBAR_PANE_CTRL_& operator=(_STATUSBAR_PANE_CTRL_&&) noexcept = default;
	};
	
	std::vector<_STATUSBAR_PANE_CTRL_> m_arrPaneControls;
	
	_STATUSBAR_PANE_* GetPanePtr(int nIndex) const noexcept
	{
		ASSERT((nIndex >= 0 && nIndex < m_nCount) || m_nCount == 0);
		return static_cast<_STATUSBAR_PANE_*>(m_pData) + nIndex;
	}
	
	bool PaneInfoGet(int nIndex, _STATUSBAR_PANE_* pPane) const noexcept;
	bool PaneInfoSet(int nIndex, const _STATUSBAR_PANE_& pPane) noexcept;
	
	void RepositionControls() noexcept;
	
	// Generated message map functions
protected:
	//{{AFX_MSG(WinMTRStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct) noexcept;
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept final override;
};


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace {
	class deferWindowPos {
		HDWP m_hdwp;
	public:
		deferWindowPos(int nNumWindows) noexcept
			:m_hdwp(::BeginDeferWindowPos(nNumWindows)) {
		}

		~deferWindowPos() noexcept
		{
			if (*this) {
				VERIFY(EndDeferWindowPos(m_hdwp));
			}
		}

		/// <summary>
		/// Indicates if we can defer windows or not, if any of the methods returned nullptr
		/// then the operation should be aborted and no further methods called.
		/// </summary>
		explicit operator bool() const noexcept {
			return m_hdwp != nullptr;
		}

		void defer(_In_ HWND hWnd,
			_In_opt_ HWND hWndInsertAfter,
			_In_ int x,
			_In_ int y,
			_In_ int cx,
			_In_ int cy,
			_In_ UINT uFlags) noexcept {
			if (*this) {
				m_hdwp = DeferWindowPos(m_hdwp, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
			}
		}
	};
}


BEGIN_MESSAGE_MAP(WinMTRStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(WinMTRStatusBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// WinMTRStatusBar message handlers
//////////////////////////////////////////////////////////////////////////

int WinMTRStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) noexcept
{
	if (CStatusBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////

LRESULT WinMTRStatusBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	LRESULT lResult = CStatusBar::WindowProc(message, wParam, lParam);
	if (message == WM_SIZE) {
		RepositionControls();
	}
	return lResult;
}

//////////////////////////////////////////////////////////////////////////

void WinMTRStatusBar::RepositionControls() noexcept
{
	deferWindowPos dwp(static_cast<int>(m_arrPaneControls.size()));

	CRect rcClient;
	GetClientRect(&rcClient);
	for (const auto& cntl : m_arrPaneControls)
	{
		int   iIndex = CommandToIndex(cntl.nID);
		HWND hWnd = cntl.hWnd;
		const auto dpi = GetDpiForWindow(hWnd);
		CRect rcPane;
		GetItemRect(iIndex, &rcPane);

		// CStatusBar::GetItemRect() sometimes returns invalid size 
		// of the last pane - we will re-compute it
		int cx = ::GetSystemMetricsForDpi(SM_CXEDGE, dpi);
		DWORD dwPaneStyle = GetPaneStyle(iIndex);
		if (iIndex == (m_nCount - 1))
		{
			if ((dwPaneStyle & SBPS_STRETCH) == 0)
			{
				UINT nID, nStyle;
				int  cxWidth;
				GetPaneInfo(iIndex, nID, nStyle, cxWidth);
				rcPane.right = rcPane.left + cxWidth + cx * 3;
			} // if( (dwPaneStyle & SBPS_STRETCH ) == 0 )
			else
			{
				CRect rcClient;
				GetClientRect(&rcClient);
				rcPane.right = rcClient.right;
				if ((GetStyle() & SBARS_SIZEGRIP) == SBARS_SIZEGRIP)
				{
					int cxSmIcon = ::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
					rcPane.right -= cxSmIcon + cx;
				} // if( (GetStyle() & SBARS_SIZEGRIP) == SBARS_SIZEGRIP )
			} // else from if( (dwPaneStyle & SBPS_STRETCH ) == 0 )
		} // if( iIndex == (m_nCount-1) )

		if ((GetPaneStyle(iIndex) & SBPS_NOBORDERS) == 0) {
			rcPane.DeflateRect(cx, cx);
		}
		else {
			rcPane.DeflateRect(cx, 1, cx, 1);
		}

		if (hWnd && ::IsWindow(hWnd)) {
			dwp.defer(
				hWnd,
				nullptr,
				rcPane.left,
				rcPane.top,
				rcPane.Width(),
				rcPane.Height(),
				SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
			);

			::RedrawWindow(
				hWnd,
				nullptr,
				nullptr,
				RDW_INVALIDATE | RDW_UPDATENOW
				| RDW_ERASE | RDW_ERASENOW
			);

		} // if (hWnd && ::IsWindow(hWnd)){ 
	}
};

//////////////////////////////////////////////////////////////////////////

BOOL WinMTRStatusBar::AddPane(
	UINT nID,	// ID of the  pane
	int nIndex	// index of the pane
)
{
	if (nIndex < 0 || nIndex > m_nCount) {
		ASSERT(FALSE);
		return FALSE;
	}

	if (CommandToIndex(nID) != -1) {
		ASSERT(FALSE);
		return FALSE;
	}

	std::vector<_STATUSBAR_PANE_> arrPanesTmp;
	arrPanesTmp.reserve(static_cast<size_t>(m_nCount) + 1);
	for (int iIndex = 0; iIndex < m_nCount + 1; iIndex++)
	{
		_STATUSBAR_PANE_ pNewPane;

		if (iIndex == nIndex) {
			pNewPane.nID = nID;
			pNewPane.nStyle = SBPS_NORMAL;
		}
		else {
			int idx = iIndex;
			if (iIndex > nIndex) idx--;

			_STATUSBAR_PANE_* pOldPane = GetPanePtr(idx);
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
	SetIndicators(IDArray);
	for (int iIndex = 0; const auto & pane : arrPanesTmp) {
		if (iIndex != nIndex) {
			PaneInfoSet(iIndex, pane);
		}

		++iIndex;
	}


	RepositionControls();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

BOOL WinMTRStatusBar::RemovePane(
	UINT nID	// ID of the pane
)
{
	if (CommandToIndex(nID) == -1 || m_nCount == 1) {
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
	for (const auto& pane : arrPanesTmp) {
		IDArray.emplace_back(pane.nID);
	}

	// set the indicators
	SetIndicators(IDArray);
	// free memory

	for (int nIndex = 0; const auto & pane : arrPanesTmp) {
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
	if (CommandToIndex(nID) == -1) {
		return FALSE;
	}

	m_arrPaneControls.emplace_back(hWnd, nID, bAutoDestroy);

	RepositionControls();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

bool WinMTRStatusBar::PaneInfoGet(int nIndex, _STATUSBAR_PANE_* pPane) const noexcept
{
	if (nIndex < m_nCount && nIndex >= 0)
	{
		GetPaneInfo(nIndex, pPane->nID, pPane->nStyle, pPane->cxText);
		CString strPaneText;
		GetPaneText(nIndex, strPaneText);
		// not actually UB since strText is a CString
		// TODO: need to figure out why the cast
		pPane->strText = LPCTSTR(strPaneText);
		return true;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool WinMTRStatusBar::PaneInfoSet(int nIndex, const _STATUSBAR_PANE_& pPane) noexcept
{
	if (nIndex < m_nCount && nIndex >= 0) {
		SetPaneInfo(nIndex, pPane.nID, pPane.nStyle, pPane.cxText);
		SetPaneText(nIndex, static_cast<LPCTSTR>(pPane.strText));
		return true;
	}
	return false;
}

	

WinMTRStatusBar::_STATUSBAR_PANE_CTRL_::~_STATUSBAR_PANE_CTRL_() noexcept
{
	if (this->hWnd && ::IsWindow(this->hWnd)) {
		::ShowWindow(this->hWnd, SW_HIDE);
		if (this->bAutoDestroy) {
			::DestroyWindow(this->hWnd);
		}
	}
}
//#endif // WINMTRSTATUSBAR_H_
