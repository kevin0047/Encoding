
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

	// 기존 기록을 메모리에 로드
	LoadAllConversionRecords();

	int nTotal = (int)m_arrFileInfo.GetSize();
	m_progress.SetRange(0, nTotal);
	m_progress.SetPos(0);

	int nConverted = 0;
	for (int i = 0; i < nTotal; i++)
	{
		const EncodingInfo& info = m_arrFileInfo[i];
		if (info.encoding == _T("ANSI") || info.encoding == _T("EUC-KR"))
		{
			if (ConvertFileEncoding(info.filePath, info.encoding, _T("UTF-8")))
			{
				// 변환 성공 시 원본 인코딩 기록
				SaveConversionRecord(info.filePath, info.encoding);
				nConverted++;
			}
		}
		m_progress.SetPos(i + 1);
	}

	// 모든 기록을 파일에 저장
	SaveAllConversionRecords();

	CString strMsg;
	strMsg.Format(_T("UTF-8 변환이 완료되었습니다. (%d개 파일 변환)"), nConverted);
	AfxMessageBox(strMsg);
	OnBnClickedButtonScan(); // 재스캔
}

void CEncodingDlg::OnBnClickedButtonToAnsi()
{
	if (m_arrFileInfo.GetSize() == 0)
	{
		AfxMessageBox(_T("먼저 스캔을 실행해주세요."));
		return;
	}

	if (AfxMessageBox(_T("UTF-8 파일들을 원본 인코딩으로 되돌리시겠습니까?"), MB_YESNO) != IDYES)
		return;

	// 모든 기록을 메모리에 로드
	LoadAllConversionRecords();

	int nTotal = (int)m_arrFileInfo.GetSize();
	m_progress.SetRange(0, nTotal);
	m_progress.SetPos(0);

	int nConverted = 0;
	int nSkipped = 0;
	for (int i = 0; i < nTotal; i++)
	{
		const EncodingInfo& info = m_arrFileInfo[i];
		if (info.encoding == _T("UTF-8"))
		{
			// 변환 기록에서 원본 인코딩 확인 (이제 메모리에서 빠르게 조회)
			CString originalEncoding = LoadOriginalEncoding(info.filePath);
			if (!originalEncoding.IsEmpty())
			{
				// 원본 인코딩으로 되돌리기
				if (ConvertFileEncoding(info.filePath, _T("UTF-8"), originalEncoding))
				{
					// 변환 성공 시 기록 삭제 (메모리에서)
					DeleteConversionRecord(info.filePath);
					nConverted++;
				}
			}
			else
			{
				// 기록이 없는 경우 - 원래부터 UTF-8이었거나 다른 도구로 변환된 파일
				nSkipped++;
			}
		}
		m_progress.SetPos(i + 1);
	}

	// 변경된 기록을 파일에 저장
	SaveAllConversionRecords();

	CString strMsg;
	if (nSkipped > 0)
	{
		strMsg.Format(_T("원본 인코딩 복원이 완료되었습니다.\n변환됨: %d개\n건너뜀: %d개 (기록 없음)"), nConverted, nSkipped);
	}
	else
	{
		strMsg.Format(_T("원본 인코딩 복원이 완료되었습니다. (%d개 파일 변환)"), nConverted);
	}
	AfxMessageBox(strMsg);
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
				else if (encoding == _T("ANSI") || encoding == _T("EUC-KR"))
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
	bool hasUtf8Multibyte = false;

	for (int i = 0; i < (int)bytesRead; i++)
	{
		BYTE b = data[i];
		if (b & 0x80) // 멀티바이트 문자
		{
			hasUtf8Multibyte = true;
			int expectedBytes = 0;
			if ((b & 0xE0) == 0xC0) expectedBytes = 1;
			else if ((b & 0xF0) == 0xE0) expectedBytes = 2;
			else if ((b & 0xF8) == 0xF0) expectedBytes = 3;
			else { isValidUtf8 = false; break; }

			// 후속 바이트들이 10xxxxxx 패턴인지 확인
			for (int j = 1; j <= expectedBytes && (i + j) < (int)bytesRead; j++)
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

	// UTF-8인지 확인
	if (isValidUtf8 && hasUtf8Multibyte)
	{
		return _T("UTF-8");
	}

	// EUC-KR/CP949 패턴 체크 (한글)
	bool hasKorean = false;
	for (int i = 0; i < (int)bytesRead - 1; i++)
	{
		BYTE b1 = data[i];
		BYTE b2 = data[i + 1];

		// EUC-KR 한글 범위 체크
		// 완성형 한글: 0xB0A1 ~ 0xC8FE
		// 조합형 한글: 0x81 ~ 0xFE의 첫 바이트
		if ((b1 >= 0xB0 && b1 <= 0xC8) ||
			(b1 >= 0x81 && b1 <= 0xFE && b2 >= 0x41 && b2 <= 0xFE))
		{
			hasKorean = true;
			break;
		}
	}

	if (hasKorean)
	{
		return _T("EUC-KR");
	}

	// ASCII나 확장 ASCII
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

		// 소스 인코딩에서 유니코드로 변환
		if (fromEncoding == _T("UTF-8"))
		{
			// UTF-8에서 변환
			int bomOffset = 0;
			if (fileSize >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
				bomOffset = 3;

			int wideLen = MultiByteToWideChar(CP_UTF8, 0,
				(char*)data.GetData() + bomOffset, (int)(fileSize - bomOffset), NULL, 0);
			if (wideLen > 0)
			{
				wideText.GetBuffer(wideLen);
				MultiByteToWideChar(CP_UTF8, 0,
					(char*)data.GetData() + bomOffset, (int)(fileSize - bomOffset),
					wideText.GetBuffer(), wideLen);
				wideText.ReleaseBuffer(wideLen);
			}
		}
		else if (fromEncoding == _T("EUC-KR"))
		{
			// EUC-KR(CP949)에서 변환 - 명시적으로 CP949 사용
			int wideLen = MultiByteToWideChar(949, 0,
				(char*)data.GetData(), (int)fileSize, NULL, 0);
			if (wideLen > 0)
			{
				wideText.GetBuffer(wideLen);
				MultiByteToWideChar(949, 0,
					(char*)data.GetData(), (int)fileSize,
					wideText.GetBuffer(), wideLen);
				wideText.ReleaseBuffer(wideLen);
			}
		}
		else if (fromEncoding == _T("ANSI"))
		{
			// ANSI에서 변환 - 시스템 기본 코드페이지 사용
			int wideLen = MultiByteToWideChar(CP_ACP, 0,
				(char*)data.GetData(), (int)fileSize, NULL, 0);
			if (wideLen > 0)
			{
				wideText.GetBuffer(wideLen);
				MultiByteToWideChar(CP_ACP, 0,
					(char*)data.GetData(), (int)fileSize,
					wideText.GetBuffer(), wideLen);
				wideText.ReleaseBuffer(wideLen);
			}
		}

		// 변환된 텍스트가 없으면 실패
		if (wideText.IsEmpty())
		{
			// 백업 파일로 복원
			CopyFile(backupPath, filePath, FALSE);
			DeleteFile(backupPath);
			return false;
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
			if (utf8Len > 1) // null terminator 포함
			{
				CStringA utf8Text;
				utf8Text.GetBuffer(utf8Len);
				WideCharToMultiByte(CP_UTF8, 0, wideText, -1, utf8Text.GetBuffer(), utf8Len, NULL, NULL);
				utf8Text.ReleaseBuffer(utf8Len - 1); // null terminator 제외

				file.Write(utf8Text, utf8Text.GetLength());
			}
		}
		else if (toEncoding == _T("EUC-KR"))
		{
			// EUC-KR(CP949)로 변환
			int eucLen = WideCharToMultiByte(949, 0, wideText, -1, NULL, 0, NULL, NULL);
			if (eucLen > 1)
			{
				CStringA eucText;
				eucText.GetBuffer(eucLen);
				WideCharToMultiByte(949, 0, wideText, -1, eucText.GetBuffer(), eucLen, NULL, NULL);
				eucText.ReleaseBuffer(eucLen - 1);

				file.Write(eucText, eucText.GetLength());
			}
		}
		else if (toEncoding == _T("ANSI"))
		{
			// ANSI로 변환
			int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideText, -1, NULL, 0, NULL, NULL);
			if (ansiLen > 1)
			{
				CStringA ansiText;
				ansiText.GetBuffer(ansiLen);
				WideCharToMultiByte(CP_ACP, 0, wideText, -1, ansiText.GetBuffer(), ansiLen, NULL, NULL);
				ansiText.ReleaseBuffer(ansiLen - 1);

				file.Write(ansiText, ansiText.GetLength());
			}
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

	strCount.Format(_T("EUC-KR/ANSI: %d개"), m_nAnsiCount);
	SetDlgItemText(IDC_STATIC_ANSI_COUNT, strCount);

	strCount.Format(_T("기타: %d개"), m_nOtherCount);
	SetDlgItemText(IDC_STATIC_OTHER_COUNT, strCount);
}

bool CEncodingDlg::IsSourceFile(const CString& fileName)
{
	CString ext = fileName.Right(4).MakeLower();
	CString ext3 = fileName.Right(3).MakeLower();
	CString ext2 = fileName.Right(2).MakeLower();

	// C/C++ 소스 파일들
	if (ext == _T(".cpp") || ext == _T(".cxx") || ext == _T(".cc") ||
		ext2 == _T(".c"))
		return true;

	// 헤더 파일들
	if (ext2 == _T(".h") || ext == _T(".hpp") || ext == _T(".hxx"))
		return true;

	// 리소스 파일들
	if (ext3 == _T(".rc") || ext == _T(".rc2") || ext == _T(".rct") ||
		ext == _T(".rgs") || ext == _T(".idl") || ext == _T(".def"))
		return true;

	// 텍스트 파일들
	if (ext == _T(".txt") || ext3 == _T(".md") || ext == _T(".ini") ||
		ext == _T(".cfg") || ext == _T(".log"))
		return true;

	// 기타 소스 파일들
	if (ext == _T(".java") || ext3 == _T(".py") || ext3 == _T(".js") ||
		ext3 == _T(".ts") || ext3 == _T(".cs") || ext3 == _T(".go") ||
		ext == _T(".php") || ext3 == _T(".rb") || ext3 == _T(".pl") ||
		ext == _T(".sql") || ext == _T(".xml") || ext == _T(".htm") ||
		fileName.Right(5).MakeLower() == _T(".html"))
		return true;

	return false;
}

CString CEncodingDlg::GetRecordFilePath()
{
	// 실행 파일과 같은 디렉토리에 기록 파일 생성
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	CString strAppPath = szPath;
	int nPos = strAppPath.ReverseFind(_T('\\'));
	if (nPos != -1)
		strAppPath = strAppPath.Left(nPos + 1);

	return strAppPath + _T("EncodingConversionRecord.txt");
}

void CEncodingDlg::SaveConversionRecord(const CString& filePath, const CString& originalEncoding)
{
	// 메모리 캐시에 저장
	m_mapConversionCache.SetAt(filePath, originalEncoding);
}

CString CEncodingDlg::LoadOriginalEncoding(const CString& filePath)
{
	CString originalEncoding;
	if (m_mapConversionCache.Lookup(filePath, originalEncoding))
	{
		return originalEncoding;
	}
	return _T("");
}

void CEncodingDlg::DeleteConversionRecord(const CString& filePath)
{
	// 메모리 캐시에서 삭제
	m_mapConversionCache.RemoveKey(filePath);
}

void CEncodingDlg::LoadAllConversionRecords()
{
	m_mapConversionCache.RemoveAll();
	CString recordPath = GetRecordFilePath();

	try
	{
		CStdioFile file;
		if (file.Open(recordPath, CFile::modeRead))
		{
			CString line;
			while (file.ReadString(line))
			{
				int delimPos = line.Find(_T('|'));
				if (delimPos != -1)
				{
					CString filePath = line.Left(delimPos);
					CString encoding = line.Mid(delimPos + 1);
					m_mapConversionCache.SetAt(filePath, encoding);
				}
			}
			file.Close();
		}
	}
	catch (...)
	{
		// 에러 시 빈 캐시 유지
	}
}

void CEncodingDlg::SaveAllConversionRecords()
{
	CString recordPath = GetRecordFilePath();

	try
	{
		CStdioFile file;
		if (file.Open(recordPath, CFile::modeCreate | CFile::modeWrite))
		{
			POSITION pos = m_mapConversionCache.GetStartPosition();
			while (pos != NULL)
			{
				CString filePath, encoding;
				m_mapConversionCache.GetNextAssoc(pos, filePath, encoding);

				CString record;
				record.Format(_T("%s|%s\n"), filePath, encoding);
				file.WriteString(record);
			}
			file.Close();
		}
	}
	catch (...)
	{
		// 에러 무시
	}
}

void CEncodingDlg::ClearConversionCache()
{
	m_mapConversionCache.RemoveAll();
}

