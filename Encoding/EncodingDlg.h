
// EncodingDlg.h: 헤더 파일
//

#pragma once


struct EncodingInfo {
	CString filePath;
	CString encoding;
	DWORD fileSize;
};

// CEncodingDlg 대화 상자
class CEncodingDlg : public CDialogEx
{
// 생성입니다.
public:
	CEncodingDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENCODING_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 멤버 변수
public:
	CString m_strPath;
	CListCtrl m_listResults;
	CProgressCtrl m_progress;

	int m_nUtf8Count;
	int m_nAnsiCount;
	int m_nOtherCount;

	CArray<EncodingInfo> m_arrFileInfo;
	CMap<CString, LPCTSTR, CString, LPCTSTR> m_mapConversionCache;

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonScan();
	afx_msg void OnBnClickedButtonToUtf8();
	afx_msg void OnBnClickedButtonToAnsi();

	// 사용자 정의 함수
	void ScanDirectory(const CString& strPath);
	CString DetectEncoding(const CString& filePath);
	bool ConvertFileEncoding(const CString& filePath, const CString& fromEncoding, const CString& toEncoding);
	void UpdateCountDisplay();
	bool IsSourceFile(const CString& fileName);

	// 변환 기록 관련 함수
	void SaveConversionRecord(const CString& filePath, const CString& originalEncoding);
	CString LoadOriginalEncoding(const CString& filePath);
	void DeleteConversionRecord(const CString& filePath);
	CString GetRecordFilePath();

	// 성능 개선 함수
	void LoadAllConversionRecords();
	void SaveAllConversionRecords();
	void ClearConversionCache();

	// 한글 경로 검사 함수
	bool HasKoreanPath(const CString& path);

	DECLARE_MESSAGE_MAP()
};
