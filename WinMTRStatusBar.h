#ifndef WINMTRSTATUSBAR_H_
#define WINMTRSTATUSBAR_H_

#include <vector>

class WinMTRStatusBar : public CStatusBar
{
// Construction
public:

// Attributes
public:

	// Operations
public:
	
	int GetPanesCount() const{
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
	
	void DisableControl( int nIndex, BOOL bDisable=TRUE)
	{
		UINT uItemID = GetItemID(nIndex);
		for (const auto & cntl : m_arrPaneControls){
			if( uItemID == cntl.nID ){
				if(cntl.hWnd && ::IsWindow(cntl.hWnd) ) {
					::EnableWindow(cntl.hWnd, bDisable);
				}
			}
		}
	}

	void SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth)
	{
		CStatusBar::SetPaneInfo(nIndex, nID, nStyle, cxWidth);
		BOOL bDisabled = ((nStyle&SBPS_DISABLED) == 0);
		DisableControl(nIndex, bDisabled);
	}

	void SetPaneStyle(int nIndex, UINT nStyle)
	{
		CStatusBar::SetPaneStyle(nIndex, nStyle);
		BOOL bDisabled = ((nStyle&SBPS_DISABLED) == 0);
		DisableControl(nIndex, bDisabled);
	}
	
	BOOL SetIndicators(std::span<UINT> indicators) {
		return static_cast<CStatusBar*>(this)->SetIndicators(indicators.data(), gsl::narrow_cast<int>(indicators.size()));
	}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WinMTRStatusBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~WinMTRStatusBar() {};

protected:

	struct _STATUSBAR_PANE_
	{
		_STATUSBAR_PANE_(){
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
		_STATUSBAR_PANE_CTRL_(HWND hWnd, UINT nID, bool bAutoDestroy)
			:hWnd(hWnd),nID(nID),bAutoDestroy(bAutoDestroy){}
		_STATUSBAR_PANE_CTRL_();
		_STATUSBAR_PANE_CTRL_(const _STATUSBAR_PANE_CTRL_&) = delete;
		~_STATUSBAR_PANE_CTRL_() noexcept;
		_STATUSBAR_PANE_CTRL_(_STATUSBAR_PANE_CTRL_&&) = default;
		_STATUSBAR_PANE_CTRL_& operator=(_STATUSBAR_PANE_CTRL_&&) = default;
	};
	
	std::vector<_STATUSBAR_PANE_CTRL_> m_arrPaneControls;
	
	_STATUSBAR_PANE_* GetPanePtr(int nIndex) const
	{
		ASSERT((nIndex >= 0 && nIndex < m_nCount) || m_nCount == 0);
		return ((_STATUSBAR_PANE_*)m_pData) + nIndex;
	}
	
	bool PaneInfoGet(int nIndex, _STATUSBAR_PANE_* pPane);
	bool PaneInfoSet(int nIndex, const _STATUSBAR_PANE_& pPane);
	
	void RepositionControls() noexcept;
	
	// Generated message map functions
protected:
	//{{AFX_MSG(WinMTRStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct) noexcept;
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept final override;
};

#endif // WINMTRSTATUSBAR_H_
