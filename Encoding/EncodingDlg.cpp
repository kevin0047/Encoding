
// EncodingDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Encoding.h"
#include "EncodingDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEncodingDlg 대화 상자



CEncodingDlg::CEncodingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ENCODING_DIALOG, pParent)
	, m_strPath(_T(""))
	, m_nUtf8Count(0)
	, m_nAnsiCount(0)
	, m_nOtherCount(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEncodingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
	DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
}

BEGIN_MESSAGE_MAP(CEncodingDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CEncodingDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CEncodingDlg::OnBnClickedButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_TO_UTF8, &CEncodingDlg::OnBnClickedButtonToUtf8)
	ON_BN_CLICKED(IDC_BUTTON_TO_ANSI, &CEncodingDlg::OnBnClickedButtonToAnsi)
END_MESSAGE_MAP()


// CEncodingDlg 메시지 처리기

BOOL CEncodingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// 리스트 컨트롤 초기화
	m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listResults.InsertColumn(0, _T("파일 경로"), LVCFMT_LEFT, 300);
	m_listResults.InsertColumn(1, _T("인코딩"), LVCFMT_LEFT, 100);
	m_listResults.InsertColumn(2, _T("크기"), LVCFMT_RIGHT, 80);

	// 진행률 표시줄 초기화
	m_progress.SetRange(0, 100);
	m_progress.SetPos(0);

	// 카운트 초기화
	UpdateCountDisplay();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CEncodingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CEncodingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CEncodingDlg::OnBnClickedButtonBrowse()
{
	CFolderPickerDialog dlg;
	if (dlg.DoModal() == IDOK)
	{
		m_strPath = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CEncodingDlg::OnBnClickedButtonScan()
{
	UpdateData(TRUE);

	if (m_strPath.IsEmpty())
	{
		AfxMessageBox(_T("스캔할 경로를 선택해주세요."));
		return;
	}

	// 리스트 초기화
	m_listResults.DeleteAllItems();
	m_arrFileInfo.RemoveAll();
	m_nUtf8Count = m_nAnsiCount = m_nOtherCount = 0;

	// 스캔 시작
	ScanDirectory(m_strPath);
	UpdateCountDisplay();
}

void CEncodingDlg::OnBnClickedButtonToUtf8()
{
	if (m_arrFileInfo.GetSize() == 0)
	{
		AfxMessageBox(_T("먼저 스캔을 실행해주세요."));
		return;
	}

	if (AfxMessageBox(_T("모든 파일을 UTF-8로 변환하시겠습니까?"), MB_YESNO) != IDYES)
		return;

	int nTotal = (int)m_arrFileInfo.GetSize();
	m_progress.SetRange(0, nTotal);
	m_progress.SetPos(0);

	for (int i = 0; i < nTotal; i++)
	{
		const EncodingInfo& info = m_arrFileInfo[i];
		if (info.encoding == _T("ANSI"))
		{
			ConvertFileEncoding(info.filePath, info.encoding, _T("UTF-8"));
		}
		m_progress.SetPos(i + 1);
	}

	AfxMessageBox(_T("UTF-8 변환이 완료되었습니다."));
	OnBnClickedButtonScan(); // 재스캔
}

void CEncodingDlg::OnBnClickedButtonToAnsi()
{
	if (m_arrFileInfo.GetSize() == 0)
	{
		AfxMessageBox(_T("먼저 스캔을 실행해주세요."));
		return;
	}

	if (AfxMessageBox(_T("모든 파일을 ANSI로 변환하시겠습니까?"), MB_YESNO) != IDYES)
		return;

	int nTotal = (int)m_arrFileInfo.GetSize();
	m_progress.SetRange(0, nTotal);
	m_progress.SetPos(0);

	for (int i = 0; i < nTotal; i++)
	{
		const EncodingInfo& info = m_arrFileInfo[i];
		if (info.encoding == _T("UTF-8"))
		{
			ConvertFileEncoding(info.filePath, info.encoding, _T("ANSI"));
		}
		m_progress.SetPos(i + 1);
	}

	AfxMessageBox(_T("ANSI 변환이 완료되었습니다."));
	OnBnClickedButtonScan(); // 재스캔
}

void CEncodingDlg::ScanDirectory(const CString& strPath)
{
	CFileFind finder;
	CString strWildcard = strPath + _T("\\*.*");

	BOOL bWorking = finder.FindFile(strWildcard);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		if (finder.IsDots())
			continue;

		if (finder.IsDirectory())
		{
			// 재귀적으로 하위 디렉토리 스캔
			ScanDirectory(finder.GetFilePath());
		}
		else
		{
			CString fileName = finder.GetFileName();
			if (IsSourceFile(fileName))
			{
				CString filePath = finder.GetFilePath();
				CString encoding = DetectEncoding(filePath);

				EncodingInfo info;
				info.filePath = filePath;
				info.encoding = encoding;
				info.fileSize = (DWORD)finder.GetLength();

				m_arrFileInfo.Add(info);

				// 리스트에 추가
				int nIndex = m_listResults.InsertItem(m_listResults.GetItemCount(), filePath);
				m_listResults.SetItemText(nIndex, 1, encoding);

				CString strSize;
				strSize.Format(_T("%d"), info.fileSize);
				m_listResults.SetItemText(nIndex, 2, strSize);

				// 카운트 증가
				if (encoding == _T("UTF-8"))
					m_nUtf8Count++;
				else if (encoding == _T("ANSI"))
					m_nAnsiCount++;
				else
					m_nOtherCount++;
			}
		}
	}

	finder.Close();
}

CString CEncodingDlg::DetectEncoding(const CString& filePath)
{
	CFile file;
	if (!file.Open(filePath, CFile::modeRead))
		return _T("Unknown");

	ULONGLONG fileSize = file.GetLength();
	if (fileSize == 0)
	{
		file.Close();
		return _T("Empty");
	}

	// BOM 체크를 위해 처음 몇 바이트 읽기
	BYTE buffer[4] = {0};
	UINT bytesRead = file.Read(buffer, min(4, (UINT)fileSize));

	// UTF-8 BOM 체크 (EF BB BF)
	if (bytesRead >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
	{
		file.Close();
		return _T("UTF-8");
	}

	// UTF-16 LE BOM 체크 (FF FE)
	if (bytesRead >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE)
	{
		file.Close();
		return _T("UTF-16 LE");
	}

	// UTF-16 BE BOM 체크 (FE FF)
	if (bytesRead >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF)
	{
		file.Close();
		return _T("UTF-16 BE");
	}

	// BOM이 없는 경우 더 많은 내용을 읽어서 판단
	file.SeekToBegin();
	const int SAMPLE_SIZE = min(1024, (int)fileSize);
	CByteArray data;
	data.SetSize(SAMPLE_SIZE);

	bytesRead = file.Read(data.GetData(), SAMPLE_SIZE);
	file.Close();

	// UTF-8 패턴 체크
	bool isValidUtf8 = true;
	for (int i = 0; i < bytesRead; i++)
	{
		BYTE b = data[i];
		if (b & 0x80) // 멀티바이트 문자
		{
			int expectedBytes = 0;
			if ((b & 0xE0) == 0xC0) expectedBytes = 1;
			else if ((b & 0xF0) == 0xE0) expectedBytes = 2;
			else if ((b & 0xF8) == 0xF0) expectedBytes = 3;
			else { isValidUtf8 = false; break; }

			// 후속 바이트들이 10xxxxxx 패턴인지 확인
			for (int j = 1; j <= expectedBytes && (i + j) < bytesRead; j++)
			{
				if ((data[i + j] & 0xC0) != 0x80)
				{
					isValidUtf8 = false;
					break;
				}
			}
			if (!isValidUtf8) break;
			i += expectedBytes;
		}
	}

	if (isValidUtf8)
	{
		// 한글이 포함되어 있으면서 UTF-8 패턴이면 UTF-8
		for (int i = 0; i < bytesRead - 2; i++)
		{
			if ((data[i] & 0xE0) == 0xE0) // 3바이트 UTF-8 (한글 포함)
			{
				return _T("UTF-8");
			}
		}
	}

	// 기본적으로 ANSI로 판단
	return _T("ANSI");
}

bool CEncodingDlg::ConvertFileEncoding(const CString& filePath, const CString& fromEncoding, const CString& toEncoding)
{
	if (fromEncoding == toEncoding)
		return true;

	try
	{
		// 백업 파일 생성
		CString backupPath = filePath + _T(".backup");
		if (!CopyFile(filePath, backupPath, FALSE))
			return false;

		CFile file;
		if (!file.Open(filePath, CFile::modeRead))
			return false;

		ULONGLONG fileSize = file.GetLength();
		CByteArray data;
		data.SetSize((INT_PTR)fileSize);

		file.Read(data.GetData(), (UINT)fileSize);
		file.Close();

		CStringW wideText;
		CStringA utf8Text;

		if (fromEncoding == _T("UTF-8"))
		{
			// UTF-8에서 변환
			int bomOffset = 0;
			if (fileSize >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
				bomOffset = 3;

			int wideLen = MultiByteToWideChar(CP_UTF8, 0,
				(char*)data.GetData() + bomOffset, (int)(fileSize - bomOffset), NULL, 0);
			wideText.GetBuffer(wideLen);
			MultiByteToWideChar(CP_UTF8, 0,
				(char*)data.GetData() + bomOffset, (int)(fileSize - bomOffset),
				wideText.GetBuffer(), wideLen);
			wideText.ReleaseBuffer(wideLen);
		}
		else if (fromEncoding == _T("ANSI"))
		{
			// ANSI(CP949)에서 변환
			int wideLen = MultiByteToWideChar(CP_ACP, 0,
				(char*)data.GetData(), (int)fileSize, NULL, 0);
			wideText.GetBuffer(wideLen);
			MultiByteToWideChar(CP_ACP, 0,
				(char*)data.GetData(), (int)fileSize,
				wideText.GetBuffer(), wideLen);
			wideText.ReleaseBuffer(wideLen);
		}

		// 대상 인코딩으로 변환하여 저장
		if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite))
			return false;

		if (toEncoding == _T("UTF-8"))
		{
			// UTF-8 BOM 쓰기
			BYTE bom[3] = {0xEF, 0xBB, 0xBF};
			file.Write(bom, 3);

			// UTF-8로 변환
			int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, NULL, 0, NULL, NULL);
			utf8Text.GetBuffer(utf8Len);
			WideCharToMultiByte(CP_UTF8, 0, wideText, -1, utf8Text.GetBuffer(), utf8Len, NULL, NULL);
			utf8Text.ReleaseBuffer(utf8Len - 1); // null terminator 제외

			file.Write(utf8Text, utf8Text.GetLength());
		}
		else if (toEncoding == _T("ANSI"))
		{
			// ANSI(CP949)로 변환
			int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideText, -1, NULL, 0, NULL, NULL);
			CStringA ansiText;
			ansiText.GetBuffer(ansiLen);
			WideCharToMultiByte(CP_ACP, 0, wideText, -1, ansiText.GetBuffer(), ansiLen, NULL, NULL);
			ansiText.ReleaseBuffer(ansiLen - 1);

			file.Write(ansiText, ansiText.GetLength());
		}

		file.Close();

		// 백업 파일 삭제
		DeleteFile(backupPath);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void CEncodingDlg::UpdateCountDisplay()
{
	CString strCount;
	strCount.Format(_T("UTF-8: %d개"), m_nUtf8Count);
	SetDlgItemText(IDC_STATIC_UTF8_COUNT, strCount);

	strCount.Format(_T("ANSI: %d"), m_nAnsiCount);
	SetDlgItemText(IDC_STATIC_ANSI_COUNT, strCount);

	strCount.Format(_T("기타: %d개"), m_nOtherCount);
	SetDlgItemText(IDC_STATIC_OTHER_COUNT, strCount);
}

bool CEncodingDlg::IsSourceFile(const CString& fileName)
{
	CString ext = fileName.Right(4).MakeLower();

	// C/C++ 소스 파일들
	if (ext == _T(".cpp") || ext == _T(".cxx") || ext == _T(".cc") ||
		fileName.Right(2).MakeLower() == _T(".c"))
		return true;

	// 헤더 파일들
	if (fileName.Right(2).MakeLower() == _T(".h") ||
		ext == _T(".hpp") || ext == _T(".hxx"))
		return true;

	// 기타 소스 파일들
	if (ext == _T(".java") || ext == _T(".py") || ext == _T(".js") ||
		ext == _T(".ts") || ext == _T(".cs") || ext == _T(".go") ||
		ext == _T(".php") || ext == _T(".rb") || ext == _T(".pl") ||
		ext == _T(".sql") || ext == _T(".xml") || ext == _T(".htm") ||
		fileName.Right(5).MakeLower() == _T(".html"))
		return true;

	return false;
}

