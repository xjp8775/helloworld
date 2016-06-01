// SyncFilesListDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SyncFilesList.h"
#include "SyncFilesListDlg.h"
#include "sqlite3.h"
#include <vector>
#include <string>
#include <list>
#include <map>
#include <io.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "CppSQLite3.h"


#include "hhhh.h"

using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_MSG_CUSTOM           23421
#define SYNC_VERSION_FILENAME	"SynInfo.ini"				//文件同步的版本文件名，放在主目录下

#pragma comment( lib, "sqlite3.lib")
#pragma warning(disable:4996)
#pragma warning(disable:4101)

const int DIR_LEVEL = 1; /// zh add on 01.01 用于截取路径
int g_dirLevels = 1;


int g_totalNum = 0;

struct FILE_PART_DIR 
{
public:
	CString  strAbsoluteFile; // 绝对路径
	CString  strRelativePath; // 相对路径
	int      nSizeFile;
};

FILE_PART_DIR g_stVersionFileItem;	           //版本文件信息
CString g_strVersionPathFile = _T("");	       //版本文件路径，用于获取文件列表时区分
CString g_strAndroidVersionPathFile = _T("");

// CAssisterApp 初始化
std::vector<FILE_PART_DIR>  g_vecSyncFiles;
std::vector<CString>        g_vecSyncDir;     // 需要同步到设备文件

static int _sql_callback(void * notused, int argc, char ** argv, char ** szColName)
{
	int i;
	for ( i=0; i < argc; i++ )
	{
		printf( "%s = %s\n", szColName[i], argv[i] == 0 ? "NUL" : argv[i] );
	}

	return 0;
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CSyncFilesListDlg 对话框
static void getDirPath(CString &strDir2, CString strDir)
{
	CString strAbsolutePath = strDir;
	const char* pBuf = strAbsolutePath.GetString();
	int nPos = -1;
	int nCounter = 0;
	for(int i  = 0; i < strAbsolutePath.GetLength(); i++)
	{
		if( '\\' == pBuf[i])
		{
			nCounter++;
		}

		if ((g_dirLevels + 1) == nCounter)
		{
			nPos =  i;
			break;

		}
	}

	//---截取字符串---
	CString strOrg = strDir;
	CString strSub;
	strSub.Empty();

	if (nPos >= 0)
	{		
		strSub = strOrg.Right(strOrg.GetLength() - nPos - 1);
	}
	strSub.Replace("\\", "/");
	//int iPos1 = strSub.Find("external/movie", 0);
	//int iPos2 = strSub.Find("external/tv", 0);
	//if( iPos1 == 0 || iPos2 == 0)
	//{
	//	strSub.Replace("external", "sdcard-ext");
	//	strDir2 = "/mnt/";
	//}
	//else
	//{
		strDir2 = "/mnt/sdcard/";
	//}
	strDir2 += strSub;
}

// 获取文件的相对路径，和文件大小信息select * from filenamelist where name like '%/mnt/sdcard-ext%'
static void getFileSize(FILE_PART_DIR &fileItem, CString strDir)
{
	FILE* file = fopen( fileItem.strAbsoluteFile.GetString(), "rb");
	if (file)
	{
		fileItem.nSizeFile = filelength(fileno(file));
		fclose(file);
	}
	else
	{
		fileItem.nSizeFile = 0;
	}

	//--------zh 修改，根据DIR_LEVEL来截取路径---------------

	//--先找到需要截取的位置---
	CString strAbsolutePath = fileItem.strAbsoluteFile;
	const char* pBuf = strAbsolutePath.GetBuffer(0);
	int nPos = -1;
	int nCounter = 0;
	for(int i  = 0; i < strAbsolutePath.GetLength(); i++)
	{
		if( '\\' == pBuf[i])
		{
			nCounter++;
		}

		if ((g_dirLevels + 1) == nCounter)
		{
			nPos =  i;
			break;

		}
	}

	///---截取字符串---
	CString strOrg = fileItem.strAbsoluteFile;
	CString strSub;
	strSub.Empty();

	if (nPos >= 0)
	{		
		strSub = strOrg.Right(strOrg.GetLength() - nPos - 1);
	}
	strSub.Replace("\\", "/");
	//int iPos1 = strSub.Find("external/movie/", 0);
	//int iPos2 = strSub.Find("external/tv/", 0);
	//if( iPos1 == 0 || iPos2 == 0)
	//{
	//	fileItem.strRelativePath = "/mnt/";
	//	strSub.Replace("external", "sdcard-ext");
	//}
	//else
	//{
		fileItem.strRelativePath = "/mnt/sdcard/";
	//}
	fileItem.strRelativePath += strSub;
}

static void ListFolder( CString sPath, bool bIsGame = false)  
{  	
	CFileFind ff;  
	BOOL bFound = FALSE;
	if (bIsGame)
		bFound = ff.FindFile(sPath + "\\*.apk");
	else
		bFound = ff.FindFile(sPath + "\\*.*");

	while(bFound)//如果找到,继续
	{
		bFound = ff.FindNextFile();  
		CString sFilePath = ff.GetFilePath();

		if(ff.IsDirectory())//如果是目录,注意任何一个目录都包括.和..目录
		{
			if(!ff.IsDots())//去除.和..目录  
			{ 
				CString strDirPath = sFilePath;
				CString strDir2;
				getDirPath( strDir2, sFilePath);
				strDir2.Replace("\'", "\''");
				g_vecSyncDir.push_back( strDir2);
				ListFolder(sFilePath, bIsGame);//递归下一层目录
			}	
		}
		else 
		{
			FILE_PART_DIR stFileItem;
			stFileItem.strAbsoluteFile = sFilePath;
			getFileSize( stFileItem, sPath);
			//if (bIsGame)
			//	g_vecGameInstall.push_back(sFilePath);
			//else
			{
				if (g_strVersionPathFile != sFilePath)	//当文件不是版本文件时，记录文件，版本文件最后放入同步列表
				{
					stFileItem.strRelativePath.Replace("\'", "\''");
					g_vecSyncFiles.push_back( stFileItem);
				}
				//else
				//{	
				//	//获取版本信息内容，获取完列表后加入最后面
				//	getFileSize(g_stVersionFileItem, sPath);
				//}
			}
		}
	}

	ff.Close();  
}

// 获取Device版本信息
CString CSyncFilesListDlg::getVersion()
{
	if (g_strVersion.IsEmpty())
	{
		if (g_strVersionPathFile.IsEmpty())
		{
			CString strLocalMainDir = "";
			//if (g_strLocalDirs.GetSize() > 0)
			//	strLocalMainDir = g_strLocalDirs.GetAt(0);

			CString strLocalVersoin = strLocalMainDir;
			if (strLocalVersoin.Right(1) != "\\")
			{
				strLocalVersoin += "\\";
			}
			strLocalVersoin += SYNC_VERSION_FILENAME;
			g_strVersionPathFile = strLocalVersoin;
		}

		CFile file;
		if (!file.Open(g_strVersionPathFile, CFile::modeRead))
			return _T("");

		char szVersion[1024] = {0};

		int nRead = file.Read(szVersion, 1024);
		if (nRead > 0)
			g_strVersion = szVersion;

		file.Close();
	}

	return g_strVersion;
}

CSyncFilesListDlg::CSyncFilesListDlg(CWnd* pParent /*=NULL*/)
: CDialog(CSyncFilesListDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSyncFilesListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSyncFilesListDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_MESSAGE( WM_MSG_CUSTOM, OnTiggerProcess)
END_MESSAGE_MAP()


// CSyncFilesListDlg 消息处理程序
// 写日志
void CSyncFilesListDlg::WriteLog( std::string strLog)
{
	SYSTEMTIME st;
	CString strTime;
	GetLocalTime(&st);
	strTime.Format("%2d:%2d:%2d",st.wHour,st.wMinute,st.wSecond);
	std::string strLog1 = strTime.GetString();
	strLog1 += "  ";
	strLog1 += strLog;

	std::string strLogPath = g_strAppPath.GetString();
	strLogPath += "SyncFilesApp.Log";
	ofstream o_file;       // 输出流：将数据从内存输出其中ofstream是将数据输出到文件，因此对于文件来说是“写”


	o_file.open(strLogPath.c_str(), ios_base::app|ios_base::out);
	o_file<<strLog1.c_str()<<"\r\n";//将内容写入文本
	o_file.close(); 
}

// 获取同步信息B
void CSyncFilesListDlg::ParserSynsInfo()
{
	WriteLog( "开始解析 LocalConfig.ini");

	TCHAR strDirect[MAX_PATH] = {0};
	TCHAR strExternal[MAX_PATH] = {0};
	TCHAR lpFile[MAX_PATH] = {0};
	TCHAR SourDir[MAX_PATH*10] = {0};
	TCHAR szGameDir[MAX_PATH] = {0};
	TCHAR szApkDir[MAX_PATH] = {0};
	TCHAR DestDir[MAX_PATH] = {0};
	TCHAR Service[MAX_PATH] = {0};
	TCHAR strPort[MAX_PATH] = {0};
	CString strFile = g_strAppPath + "LocalConfig.ini";

	// 本地同步路径
	GetPrivateProfileString("ClientUpdate","LocalDir","1", SourDir, MAX_PATH, strFile);
	g_strLocalDirs = SourDir;
	//CUtil::SplitString(strDirs, "|", g_strLocalDirs);

	// 本地同步到设备的视频文件路径 2012-01-13
	GetPrivateProfileString("ClientUpdate","External","1", strExternal, MAX_PATH, strFile);
	g_strExternalDir = strExternal;

	// 本地同步Game 路径
	GetPrivateProfileString("ClientUpdate","GameDir","1", szGameDir, MAX_PATH, strFile);
	g_strGameDir = szGameDir;

	// 本地同步应用 路径 2012-01-13
	GetPrivateProfileString("ClientUpdate", "ApkDir", "1", szApkDir, MAX_PATH, strFile);
	g_strApkDir = szApkDir;

	// 设备端同步路径
	GetPrivateProfileString("ClientUpdate","RemoteDir","1", DestDir, MAX_PATH, strFile);
	g_strDeviceDir = /*g_strAppPath + */DestDir;

	// 同步木马路径
	GetPrivateProfileString("ClientUpdate","Service","1", Service, MAX_PATH, strFile);
	g_strAppService = g_strAppPath + Service;

	// 获取端口配置文件
	GetPrivateProfileString("ClientUpdate","ServerPort","1", strPort, MAX_PATH, strFile);
	g_strPortConfig = g_strAppPath + strPort;

	WriteLog( "解析 LocalConfig.ini 完成。");
}

void CSyncFilesListDlg::CaculateAppPath()
{
	char szPath[1024];
	if( GetModuleFileName( NULL, szPath, 1024))
	{
		g_strAppPath = szPath;
		g_strAppPath = g_strAppPath.Left( g_strAppPath.ReverseFind( '\\') + 1);
	}
}

CString g_strVersionSoftware;	      // 需要同步的 软件 版本
CString g_strVersionContent;	      // 需要同步的 内容 版本
CString g_strVersionFilePath;	      // 版本文件路径

static void GetVersionSoftware( CString strVersionFile)
{
	char buf[1024];                //临时保存读取出来的文件内容
	CString strVersion[2];
	ifstream infile;
	infile.open( strVersionFile.GetString());
	if( infile.is_open())          //文件打开成功,说明曾经写入过东西
	{
		int i = 0;
		while( infile.good() && !infile.eof())
		{
			memset( buf,0,1024);
			infile.getline( buf,1204);
			strVersion[i] = buf;
			i++;
		}
		infile.close();
		int iPos = strVersion[0].Find( "ContentVersion=", 0) + sizeof("ContentVersion=") - 1;
		int iLen = strVersion[0].GetLength();
		g_strVersionContent = strVersion[0].Right( iLen - iPos);	    // 需要同步的 软件 版本 

		iPos = strVersion[1].Find( "SoftVersion=", 0) + sizeof("SoftVersion=") - 1;
		iLen = strVersion[1].GetLength();
		g_strVersionSoftware  = strVersion[1].Right( iLen - iPos);	    // 需要同步的 内容 版本
	}
}

// 获取同步信息
bool CSyncFilesListDlg::ParserSynsInfo1()
{
	::DeleteFileA( g_strAppPath + "SyncFilesApp.Log");
	::DeleteFileA( g_strAppPath + "temp.db");
	TCHAR strDirect[MAX_PATH] = {0};
	TCHAR strExternal[MAX_PATH] = {0};
	TCHAR lpFile[MAX_PATH] = {0};
	TCHAR SourDir[MAX_PATH*10] = {0};
	TCHAR szGameDir[MAX_PATH] = {0};
	TCHAR szApkDir[MAX_PATH] = {0};
	TCHAR DestDir[MAX_PATH] = {0};
	TCHAR Service[MAX_PATH] = {0};
	TCHAR strPort[MAX_PATH] = {0};
	CString strFile = g_strAppPath + "LocalConfig.ini";

	// 本地同步路径
	GetPrivateProfileString("ClientUpdate","LocalDir","1", SourDir, MAX_PATH, strFile);
	CString strDirs = SourDir;
	g_strLocalPath = SourDir;
	g_strVersionFilePath = strDirs + "\\SynInfo.ini";	    // 版本文件路径
	GetVersionSoftware( g_strVersionFilePath);

	// 判断数据库文件是否存在
	WIN32_FIND_DATA FindFileData;	
	std::string strPathDB = g_strAppPath.GetString();
	strPathDB += "temp.db";
	HANDLE hFind = FindFirstFile( strPathDB.c_str(),   &FindFileData);
	if( hFind == INVALID_HANDLE_VALUE)  
	{ 
		return false;
	}
	CppSQLite3DB   *m_pdbSQLite = new CppSQLite3DB();
	m_pdbSQLite ->open( strPathDB.c_str());

	CppSQLite3Query query = m_pdbSQLite ->execQuery( "select * from fileVersion;");
	CString strVersionRecord1;
	CString strVersionRecord2;
	while( !query.eof())
	{	
		strVersionRecord1 = query.getStringField(0); // 需要同步的 软件 版本     
		strVersionRecord2 = query.getStringField(1); // 需要同步的 内容 版本
		query.nextRow();
	}
	m_pdbSQLite ->close();
	delete m_pdbSQLite;

	// 版本一致
	if( g_strVersionContent == strVersionRecord2 && g_strVersionSoftware == strVersionRecord1)
	{
		CString strError = g_strVersionContent;
		strError += "\r\n";
		strError += strVersionRecord2;
		strError += "\r\n";
		strError += g_strVersionSoftware;
		strError += "\r\n";
		strError += strVersionRecord1;
		WriteLog( strError.GetString());
		return true;
	}

	return false;
}

string CSyncFilesListDlg::GBKToUTF8( std::string& strGBK)
{ 
	string strOutUTF8 = ""; 
	wchar_t * str1; 
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0); 
	str1 = new WCHAR[n]; MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL); 
	char * str2 = new char[n]; 
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL); 
	strOutUTF8 = str2; delete[]str1; 
	str1 = NULL; 
	delete[]str2; str2 = NULL; 
	return strOutUTF8;
}

UINT WriteSQLiteThread(void *pArg)
{
	CSyncFilesListDlg *pThis = (CSyncFilesListDlg *)pArg;
	// TODO: 在此添加额外的初始化代码
	//pThis ->CaculateAppPath();
	pThis ->ParserSynsInfo();                  // 2,获取需要同步的信息

	long t1=GetTickCount();//程序段结束后取得系统运行时间(ms)
	//获取要同步的文件，不包含版本文件
	ListFolder( pThis ->g_strLocalDirs);

	//long t2=GetTickCount();//程序段结束后取得系统运行时间(ms)
	//CString str;
	//str.Format("time:%dms",t2-t1);//前后之差即 程序运行时间
	//AfxMessageBox(str);

	BOOL bResult = ::DeleteFileA(pThis ->g_strAppPath + "temp.db");
	//if( bResult)
	{
		CString strInfo = pThis ->g_strAppPath + "temp.db";
		pThis ->WriteLog( strInfo.GetString());
		pThis ->WriteLog( "删除数据库 temp.db 完成。");
		const char * sSQL1 = "CREATE TABLE filenamelist (_id INTEGER PRIMARY KEY AUTOINCREMENT, name varchar(1000));";

		sqlite3 * db = 0;
		char * pErrMsg = 0;
		int ret = 0;
		//long t1=GetTickCount();//程序段开始前取得系统运行时间(ms)
		//Sleep(500);
		//long t2=GetTickCount();//程序段结束后取得系统运行时间(ms)
		//CString str;
		//str.Format("time:%dms",t2-t1);//前后之差即 程序运行时间
		//AfxMessageBox(str);

		// 连接数据库
		ret = sqlite3_open(pThis ->g_strAppPath +"temp.db", &db);
		if ( ret != SQLITE_OK )
		{
			fprintf(stderr, "无法打开数据库: %s", sqlite3_errmsg(db));
			return(1);
		}
		sqlite3_exec( db, "PRAGMA synchronous = OFF;", 0, 0, &pErrMsg);
		printf("数据库连接成功!\n");
		// 执行建表SQL
		// 执行建表SQL
		sqlite3_exec( db, sSQL1, 0, 0, &pErrMsg );
		if ( ret != SQLITE_OK )
		{
			fprintf(stderr, "SQL error: %s\n", pErrMsg);
			sqlite3_free(pErrMsg);
		}
		CString strFileID;
		int iLen = g_vecSyncDir.size();
		for( int i = 0; i < iLen; i++)
		{
			string strSQL = "insert into filenamelist values(";
			strFileID.Format("%d", i);
			strSQL += strFileID.GetString();
			strSQL += ", '";
			strSQL += g_vecSyncDir[i].GetString();
			strSQL += "');";
			// 执行插入记录SQL

			//USES_CONVERSION;

			//wchar_t* p = A2W(strSQL.c_str());

			//char* ptest = (char*)p;
			//sqlite3_exec( db, ptest, 0, 0, &pErrMsg);
			string strInsert = CSyncFilesListDlg::GBKToUTF8( strSQL);

			int nRet = sqlite3_exec( db, strInsert.c_str(), 0, 0, &pErrMsg);
		}

		int iLength = g_vecSyncFiles.size();
		for( int i = 0; i < iLength; i++)
		{
			string strSQL = "insert into filenamelist values(";
			strFileID.Format("%d", i + iLen);
			strSQL += strFileID.GetString();
			strSQL += ", '";
			strSQL += g_vecSyncFiles[i].strRelativePath.GetString();
			strSQL += "');";
			// 执行插入记录SQL
			//int nRet = sqlite3_exec( db, strSQL.c_str(), 0, 0, &pErrMsg);
			string strInsert = CSyncFilesListDlg::GBKToUTF8( strSQL);
			int nRet = sqlite3_exec( db, strInsert.c_str(), 0, 0, &pErrMsg);
			//USES_CONVERSION;

			//wchar_t* p = A2W(strSQL.c_str());

			//char* ptest = (char*)p;
			//sqlite3_exec( db, ptest, 0, 0, &pErrMsg);
		}

		// 查询数据表
		// sqlite3_exec( db, sSQL3, _sql_callback, 0, &pErrMsg);

		// 执行建表SQL
		const char * sSQL2 = "CREATE TABLE fileVersion( softwareVer varchar(32), contentVer varchar(32));";
		sqlite3_exec( db, sSQL2, 0, 0, &pErrMsg );
		if ( ret != SQLITE_OK )
		{
			fprintf(stderr, "SQL error: %s\n", pErrMsg);
			sqlite3_free(pErrMsg);
		}
		string strSQL = "insert into fileVersion values('";
		strSQL += g_strVersionSoftware.GetString();
		strSQL += "', '";
		strSQL += g_strVersionContent.GetString();
		strSQL += "');";
		int nRet = sqlite3_exec( db, strSQL.c_str(), 0, 0, &pErrMsg);
		// 关闭数据库
		sqlite3_close(db);
		db = 0;
	}

	::PostMessage( pThis ->m_hWnd, WM_MSG_CUSTOM, 0, 0);
	return 0;
}

LRESULT CSyncFilesListDlg::OnTiggerProcess( WPARAM wParam, LPARAM lParam)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &pi, sizeof(pi) );
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	// Start the child process
	if(CreateProcess(g_strAppPath + "PaidAssister.exe", "PaidAssister.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WriteLog( "启动 PaidAssister.exe 成功！");
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
	else 
	{
		WriteLog( "启动 PaidAssister.exe 失败!");
		HANDLE hProcess = GetCurrentProcess();//get current process
		TerminateProcess(hProcess,0);         //close process
	}
	this ->OnCancel();
	return 0;
}

static bool SetAutoStart(LPCTSTR strAppName)
{
	bool bRes = true;
	HKEY hKey; 
	//找到系统的启动项 
	LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run"; 
	//打开启动项Key 
	long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE, &hKey); 
	if(lRet == ERROR_SUCCESS) 
	{ 
		char pFileName[MAX_PATH] = {0}; 
		//得到程序自身的全路径 
		DWORD dwRet = GetModuleFileName(NULL, pFileName, MAX_PATH); 
		//添加一个子Key,并设置值 // 下面的"getip"是应用程序名字（不加后缀.exe）
		lRet = RegSetValueEx(hKey, strAppName, 0, REG_SZ, (BYTE *)pFileName, dwRet); 

		//关闭注册表 
		RegCloseKey(hKey); 
		if(lRet != ERROR_SUCCESS) 
		{ 
			bRes = false;
		} 
	}
	else bRes = false;

	return bRes;
}


static BOOL   DeleteDirectory(LPCTSTR   DirName) 
{ 
	CFileFind     tempFind;   
	char     tempFileFind[200];   
	sprintf(tempFileFind, "%s\\*.* ",DirName);   
	BOOL     IsFinded=(BOOL)tempFind.FindFile(tempFileFind);   
	while(IsFinded)   
	{   
		IsFinded=(BOOL)tempFind.FindNextFile();   
		if(!tempFind.IsDots())   //   如果不是 '. '或者 '.. ' 
		{   
			char     foundFileName[200];   
			strcpy(foundFileName,tempFind.GetFileName().GetBuffer(200));   
			if(tempFind.IsDirectory())     //是否是目录 
			{   
				char     tempDir[200];   
				sprintf(tempDir, "%s\\%s ",DirName,foundFileName);   
				DeleteDirectory(tempDir);   
			}   
			else                                           //若是文件,则删除 
			{   
				char     tempFileName[200];   
				sprintf(tempFileName, "%s\\%s ",DirName,foundFileName);   
				DeleteFile(tempFileName);   
			}   
		}   
	}   
	tempFind.Close();   
	if(!RemoveDirectory(DirName))   
	{   
		//AfxMessageBox( "删除目录失败！ ",MB_OK);   
		return     FALSE;   
	}   
	return     TRUE;   

} 

BOOL CSyncFilesListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	WriteLog( "获取应用路径。");

	CaculateAppPath();

	if (!SetAutoStart("SyncFilesList"))
	{
		WriteLog("Can't set auto start.");
	}
	WriteLog( "获取应用路径。完成");

	// 版本不一致需要重写数据库
	bool bRes = ParserSynsInfo1();
	// 先删除临时安装文件夹
	CString strTemp = g_strLocalPath + "\\gameTempInstall";
	DeleteDirectory( strTemp);

	if( bRes == false)
	{
		WriteLog( "版本不一致，重新写入同步数据库。");
		AfxBeginThread(WriteSQLiteThread,(void *)this);
		WriteLog( "数据库，写入完成！");
	}
	else
	{
		OnTiggerProcess( 0, 0);
		WriteLog( "启动同步进程完成！");
	}


	CString strPath = g_strAppPath + "Res\\Loading.jpg";
	m_imgBkg.Load( strPath);
	this ->GetDlgItem(IDOK) ->ShowWindow(SW_HIDE);
	this ->GetDlgItem(IDCANCEL) ->ShowWindow(SW_HIDE);
	SetTimer( 0, 1000*3, NULL);
	int iw = m_imgBkg.GetWidth();
	int ih = m_imgBkg.GetHeight();
	this ->MoveWindow( 0, 0, iw, ih);



	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSyncFilesListDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 绘制背景
void CSyncFilesListDlg::drawTempDC( CDC &dcTemp)
{
	CDC imgDC;
	imgDC.CreateCompatibleDC(&dcTemp);
	imgDC.SelectObject( m_imgBkg);
	dcTemp.SetStretchBltMode( HALFTONE); 
	int iw = m_imgBkg.GetWidth();
	int ih = m_imgBkg.GetHeight();
	CRect rcWindow;
	GetClientRect(rcWindow);
	dcTemp.StretchBlt( 0, 0, iw, ih, &imgDC,0, 0, iw, ih, SRCCOPY);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSyncFilesListDlg::OnPaint()
{
	CPaintDC dc(this); // 用于绘制的设备上下文
	CBitmap graph;
	CDC TempDC;	
	CRect rcClient;
	GetClientRect(&rcClient);
	TempDC.CreateCompatibleDC(&dc);
	graph.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
	CBitmap *pOldBitmap = (CBitmap*)TempDC.SelectObject(&graph);

	// 绘制滚动条移动后的界面
	drawTempDC( TempDC);
	int iw = m_imgBkg.GetWidth();
	int ih = m_imgBkg.GetHeight();
	dc.BitBlt( 0, 0, iw, ih, &TempDC, 0, 0, SRCCOPY);

	TempDC.SelectObject(pOldBitmap);
	TempDC.DeleteDC();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSyncFilesListDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

