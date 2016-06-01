// SyncFilesListDlg.cpp : ʵ���ļ�
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
#define SYNC_VERSION_FILENAME	"SynInfo.ini"				//�ļ�ͬ���İ汾�ļ�����������Ŀ¼��

#pragma comment( lib, "sqlite3.lib")
#pragma warning(disable:4996)
#pragma warning(disable:4101)

const int DIR_LEVEL = 1; /// zh add on 01.01 ���ڽ�ȡ·��
int g_dirLevels = 1;


int g_totalNum = 0;

struct FILE_PART_DIR 
{
public:
	CString  strAbsoluteFile; // ����·��
	CString  strRelativePath; // ���·��
	int      nSizeFile;
};

FILE_PART_DIR g_stVersionFileItem;	           //�汾�ļ���Ϣ
CString g_strVersionPathFile = _T("");	       //�汾�ļ�·�������ڻ�ȡ�ļ��б�ʱ����
CString g_strAndroidVersionPathFile = _T("");

// CAssisterApp ��ʼ��
std::vector<FILE_PART_DIR>  g_vecSyncFiles;
std::vector<CString>        g_vecSyncDir;     // ��Ҫͬ�����豸�ļ�

static int _sql_callback(void * notused, int argc, char ** argv, char ** szColName)
{
	int i;
	for ( i=0; i < argc; i++ )
	{
		printf( "%s = %s\n", szColName[i], argv[i] == 0 ? "NUL" : argv[i] );
	}

	return 0;
}

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	// ʵ��
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


// CSyncFilesListDlg �Ի���
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

	//---��ȡ�ַ���---
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

// ��ȡ�ļ������·�������ļ���С��Ϣselect * from filenamelist where name like '%/mnt/sdcard-ext%'
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

	//--------zh �޸ģ�����DIR_LEVEL����ȡ·��---------------

	//--���ҵ���Ҫ��ȡ��λ��---
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

	///---��ȡ�ַ���---
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

	while(bFound)//����ҵ�,����
	{
		bFound = ff.FindNextFile();  
		CString sFilePath = ff.GetFilePath();

		if(ff.IsDirectory())//�����Ŀ¼,ע���κ�һ��Ŀ¼������.��..Ŀ¼
		{
			if(!ff.IsDots())//ȥ��.��..Ŀ¼  
			{ 
				CString strDirPath = sFilePath;
				CString strDir2;
				getDirPath( strDir2, sFilePath);
				strDir2.Replace("\'", "\''");
				g_vecSyncDir.push_back( strDir2);
				ListFolder(sFilePath, bIsGame);//�ݹ���һ��Ŀ¼
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
				if (g_strVersionPathFile != sFilePath)	//���ļ����ǰ汾�ļ�ʱ����¼�ļ����汾�ļ�������ͬ���б�
				{
					stFileItem.strRelativePath.Replace("\'", "\''");
					g_vecSyncFiles.push_back( stFileItem);
				}
				//else
				//{	
				//	//��ȡ�汾��Ϣ���ݣ���ȡ���б����������
				//	getFileSize(g_stVersionFileItem, sPath);
				//}
			}
		}
	}

	ff.Close();  
}

// ��ȡDevice�汾��Ϣ
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


// CSyncFilesListDlg ��Ϣ�������
// д��־
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
	ofstream o_file;       // ������������ݴ��ڴ��������ofstream�ǽ�����������ļ�����˶����ļ���˵�ǡ�д��


	o_file.open(strLogPath.c_str(), ios_base::app|ios_base::out);
	o_file<<strLog1.c_str()<<"\r\n";//������д���ı�
	o_file.close(); 
}

// ��ȡͬ����ϢB
void CSyncFilesListDlg::ParserSynsInfo()
{
	WriteLog( "��ʼ���� LocalConfig.ini");

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

	// ����ͬ��·��
	GetPrivateProfileString("ClientUpdate","LocalDir","1", SourDir, MAX_PATH, strFile);
	g_strLocalDirs = SourDir;
	//CUtil::SplitString(strDirs, "|", g_strLocalDirs);

	// ����ͬ�����豸����Ƶ�ļ�·�� 2012-01-13
	GetPrivateProfileString("ClientUpdate","External","1", strExternal, MAX_PATH, strFile);
	g_strExternalDir = strExternal;

	// ����ͬ��Game ·��
	GetPrivateProfileString("ClientUpdate","GameDir","1", szGameDir, MAX_PATH, strFile);
	g_strGameDir = szGameDir;

	// ����ͬ��Ӧ�� ·�� 2012-01-13
	GetPrivateProfileString("ClientUpdate", "ApkDir", "1", szApkDir, MAX_PATH, strFile);
	g_strApkDir = szApkDir;

	// �豸��ͬ��·��
	GetPrivateProfileString("ClientUpdate","RemoteDir","1", DestDir, MAX_PATH, strFile);
	g_strDeviceDir = /*g_strAppPath + */DestDir;

	// ͬ��ľ��·��
	GetPrivateProfileString("ClientUpdate","Service","1", Service, MAX_PATH, strFile);
	g_strAppService = g_strAppPath + Service;

	// ��ȡ�˿������ļ�
	GetPrivateProfileString("ClientUpdate","ServerPort","1", strPort, MAX_PATH, strFile);
	g_strPortConfig = g_strAppPath + strPort;

	WriteLog( "���� LocalConfig.ini ��ɡ�");
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

CString g_strVersionSoftware;	      // ��Ҫͬ���� ��� �汾
CString g_strVersionContent;	      // ��Ҫͬ���� ���� �汾
CString g_strVersionFilePath;	      // �汾�ļ�·��

static void GetVersionSoftware( CString strVersionFile)
{
	char buf[1024];                //��ʱ�����ȡ�������ļ�����
	CString strVersion[2];
	ifstream infile;
	infile.open( strVersionFile.GetString());
	if( infile.is_open())          //�ļ��򿪳ɹ�,˵������д�������
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
		g_strVersionContent = strVersion[0].Right( iLen - iPos);	    // ��Ҫͬ���� ��� �汾 

		iPos = strVersion[1].Find( "SoftVersion=", 0) + sizeof("SoftVersion=") - 1;
		iLen = strVersion[1].GetLength();
		g_strVersionSoftware  = strVersion[1].Right( iLen - iPos);	    // ��Ҫͬ���� ���� �汾
	}
}

// ��ȡͬ����Ϣ
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

	// ����ͬ��·��
	GetPrivateProfileString("ClientUpdate","LocalDir","1", SourDir, MAX_PATH, strFile);
	CString strDirs = SourDir;
	g_strLocalPath = SourDir;
	g_strVersionFilePath = strDirs + "\\SynInfo.ini";	    // �汾�ļ�·��
	GetVersionSoftware( g_strVersionFilePath);

	// �ж����ݿ��ļ��Ƿ����
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
		strVersionRecord1 = query.getStringField(0); // ��Ҫͬ���� ��� �汾     
		strVersionRecord2 = query.getStringField(1); // ��Ҫͬ���� ���� �汾
		query.nextRow();
	}
	m_pdbSQLite ->close();
	delete m_pdbSQLite;

	// �汾һ��
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
	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//pThis ->CaculateAppPath();
	pThis ->ParserSynsInfo();                  // 2,��ȡ��Ҫͬ������Ϣ

	long t1=GetTickCount();//����ν�����ȡ��ϵͳ����ʱ��(ms)
	//��ȡҪͬ�����ļ����������汾�ļ�
	ListFolder( pThis ->g_strLocalDirs);

	//long t2=GetTickCount();//����ν�����ȡ��ϵͳ����ʱ��(ms)
	//CString str;
	//str.Format("time:%dms",t2-t1);//ǰ��֮� ��������ʱ��
	//AfxMessageBox(str);

	BOOL bResult = ::DeleteFileA(pThis ->g_strAppPath + "temp.db");
	//if( bResult)
	{
		CString strInfo = pThis ->g_strAppPath + "temp.db";
		pThis ->WriteLog( strInfo.GetString());
		pThis ->WriteLog( "ɾ�����ݿ� temp.db ��ɡ�");
		const char * sSQL1 = "CREATE TABLE filenamelist (_id INTEGER PRIMARY KEY AUTOINCREMENT, name varchar(1000));";

		sqlite3 * db = 0;
		char * pErrMsg = 0;
		int ret = 0;
		//long t1=GetTickCount();//����ο�ʼǰȡ��ϵͳ����ʱ��(ms)
		//Sleep(500);
		//long t2=GetTickCount();//����ν�����ȡ��ϵͳ����ʱ��(ms)
		//CString str;
		//str.Format("time:%dms",t2-t1);//ǰ��֮� ��������ʱ��
		//AfxMessageBox(str);

		// �������ݿ�
		ret = sqlite3_open(pThis ->g_strAppPath +"temp.db", &db);
		if ( ret != SQLITE_OK )
		{
			fprintf(stderr, "�޷������ݿ�: %s", sqlite3_errmsg(db));
			return(1);
		}
		sqlite3_exec( db, "PRAGMA synchronous = OFF;", 0, 0, &pErrMsg);
		printf("���ݿ����ӳɹ�!\n");
		// ִ�н���SQL
		// ִ�н���SQL
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
			// ִ�в����¼SQL

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
			// ִ�в����¼SQL
			//int nRet = sqlite3_exec( db, strSQL.c_str(), 0, 0, &pErrMsg);
			string strInsert = CSyncFilesListDlg::GBKToUTF8( strSQL);
			int nRet = sqlite3_exec( db, strInsert.c_str(), 0, 0, &pErrMsg);
			//USES_CONVERSION;

			//wchar_t* p = A2W(strSQL.c_str());

			//char* ptest = (char*)p;
			//sqlite3_exec( db, ptest, 0, 0, &pErrMsg);
		}

		// ��ѯ���ݱ�
		// sqlite3_exec( db, sSQL3, _sql_callback, 0, &pErrMsg);

		// ִ�н���SQL
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
		// �ر����ݿ�
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
		WriteLog( "���� PaidAssister.exe �ɹ���");
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
	else 
	{
		WriteLog( "���� PaidAssister.exe ʧ��!");
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
	//�ҵ�ϵͳ�������� 
	LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run"; 
	//��������Key 
	long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE, &hKey); 
	if(lRet == ERROR_SUCCESS) 
	{ 
		char pFileName[MAX_PATH] = {0}; 
		//�õ����������ȫ·�� 
		DWORD dwRet = GetModuleFileName(NULL, pFileName, MAX_PATH); 
		//���һ����Key,������ֵ // �����"getip"��Ӧ�ó������֣����Ӻ�׺.exe��
		lRet = RegSetValueEx(hKey, strAppName, 0, REG_SZ, (BYTE *)pFileName, dwRet); 

		//�ر�ע��� 
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
		if(!tempFind.IsDots())   //   ������� '. '���� '.. ' 
		{   
			char     foundFileName[200];   
			strcpy(foundFileName,tempFind.GetFileName().GetBuffer(200));   
			if(tempFind.IsDirectory())     //�Ƿ���Ŀ¼ 
			{   
				char     tempDir[200];   
				sprintf(tempDir, "%s\\%s ",DirName,foundFileName);   
				DeleteDirectory(tempDir);   
			}   
			else                                           //�����ļ�,��ɾ�� 
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
		//AfxMessageBox( "ɾ��Ŀ¼ʧ�ܣ� ",MB_OK);   
		return     FALSE;   
	}   
	return     TRUE;   

} 

BOOL CSyncFilesListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	WriteLog( "��ȡӦ��·����");

	CaculateAppPath();

	if (!SetAutoStart("SyncFilesList"))
	{
		WriteLog("Can't set auto start.");
	}
	WriteLog( "��ȡӦ��·�������");

	// �汾��һ����Ҫ��д���ݿ�
	bool bRes = ParserSynsInfo1();
	// ��ɾ����ʱ��װ�ļ���
	CString strTemp = g_strLocalPath + "\\gameTempInstall";
	DeleteDirectory( strTemp);

	if( bRes == false)
	{
		WriteLog( "�汾��һ�£�����д��ͬ�����ݿ⡣");
		AfxBeginThread(WriteSQLiteThread,(void *)this);
		WriteLog( "���ݿ⣬д����ɣ�");
	}
	else
	{
		OnTiggerProcess( 0, 0);
		WriteLog( "����ͬ��������ɣ�");
	}


	CString strPath = g_strAppPath + "Res\\Loading.jpg";
	m_imgBkg.Load( strPath);
	this ->GetDlgItem(IDOK) ->ShowWindow(SW_HIDE);
	this ->GetDlgItem(IDCANCEL) ->ShowWindow(SW_HIDE);
	SetTimer( 0, 1000*3, NULL);
	int iw = m_imgBkg.GetWidth();
	int ih = m_imgBkg.GetHeight();
	this ->MoveWindow( 0, 0, iw, ih);



	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// ���Ʊ���
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CSyncFilesListDlg::OnPaint()
{
	CPaintDC dc(this); // ���ڻ��Ƶ��豸������
	CBitmap graph;
	CDC TempDC;	
	CRect rcClient;
	GetClientRect(&rcClient);
	TempDC.CreateCompatibleDC(&dc);
	graph.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
	CBitmap *pOldBitmap = (CBitmap*)TempDC.SelectObject(&graph);

	// ���ƹ������ƶ���Ľ���
	drawTempDC( TempDC);
	int iw = m_imgBkg.GetWidth();
	int ih = m_imgBkg.GetHeight();
	dc.BitBlt( 0, 0, iw, ih, &TempDC, 0, 0, SRCCOPY);

	TempDC.SelectObject(pOldBitmap);
	TempDC.DeleteDC();
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CSyncFilesListDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

