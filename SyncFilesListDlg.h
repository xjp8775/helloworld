// SyncFilesListDlg.h : ͷ�ļ�
//

#pragma once
#include <atlimage.h>
#include <string>

using namespace std;

// CSyncFilesListDlg �Ի���
class CSyncFilesListDlg : public CDialog
{
// ����
public:
	CSyncFilesListDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SYNCFILESLIST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

private:
	CImage         m_imgBkg;
	void drawTempDC( CDC &dcTemp);

public:
	CString  getVersion();
    void     ParserSynsInfo();
	CString  g_strAppPath;
	void     CaculateAppPath();
	static string   GBKToUTF8( std::string& strGBK);
    bool ParserSynsInfo1();                           // ��ȡͬ����Ϣ
	void WriteLog( std::string strLog);

public:
	CString g_strLocalPath;
	CString g_strLocalDirs;
	CString g_strDeviceDir;
	CString g_strExternalDir;     // ��Ҫͬ����move&tv·��
	CString g_strGameDir;         // ��Ҫͬ����װ��Ӧ��·��
	CString g_strApkDir;          // ��Ҫͬ����װ����Ϸ·��
	CString g_strVersion;         // �汾�ַ���
	
	CString g_strPortConfig;
	CString g_strAppService;

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTiggerProcess( WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
