// SyncFilesListDlg.h : 头文件
//

#pragma once
#include <atlimage.h>
#include <string>

using namespace std;

// CSyncFilesListDlg 对话框
class CSyncFilesListDlg : public CDialog
{
// 构造
public:
	CSyncFilesListDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SYNCFILESLIST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	CImage         m_imgBkg;
	void drawTempDC( CDC &dcTemp);

public:
	CString  getVersion();
    void     ParserSynsInfo();
	CString  g_strAppPath;
	void     CaculateAppPath();
	static string   GBKToUTF8( std::string& strGBK);
    bool ParserSynsInfo1();                           // 获取同步信息
	void WriteLog( std::string strLog);

public:
	CString g_strLocalPath;
	CString g_strLocalDirs;
	CString g_strDeviceDir;
	CString g_strExternalDir;     // 需要同步的move&tv路径
	CString g_strGameDir;         // 需要同步安装的应用路径
	CString g_strApkDir;          // 需要同步安装的游戏路径
	CString g_strVersion;         // 版本字符串
	
	CString g_strPortConfig;
	CString g_strAppService;

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTiggerProcess( WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
