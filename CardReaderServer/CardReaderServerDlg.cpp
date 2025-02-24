// CardReaderServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CardReaderServer.h"
#include "CardReaderServerDlg.h"
#include "ServerSetting.h"
#include "ServerUtils.h"
#include "CustomMessage.h"
#include <set>
#include <list>
#include <map>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCardReaderServerDlg dialog

CCardReaderServerDlg::CCardReaderServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCardReaderServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCardReaderServerDlg)
	m_errors = 0;
	m_warns = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCardReaderServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCardReaderServerDlg)
	DDX_Control(pDX, IDC_EDIT_LOG, m_logWindow);
	DDX_Control(pDX, IDC_TREE_CLIENTS, m_Tree);
	DDX_Text(pDX, IDC_EDIT_ERRORS, m_errors);
	DDX_Text(pDX, IDC_EDIT_WARNS, m_warns);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCardReaderServerDlg, CDialog)
	//{{AFX_MSG_MAP(CCardReaderServerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, OnButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_RESTART, OnButtonRestart)
	ON_BN_CLICKED(IDC_BUTTON_SETTING, OnButtonSetting)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_LOG, OnButtonLog)
	ON_MESSAGE(LOG_UPDATE_MSG, UpdateLog)
	ON_MESSAGE(WM_SHOWTASK,OnShowTask)
	ON_MESSAGE(WM_HIDETASK, ToTray)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCardReaderServerDlg message handlers

BOOL CCardReaderServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	// 为树添加图标
// 	CImageList cImageList;
// 	cImageList.Create(16, 16,  ILC_COLOR32| ILC_MASK, 1, 2);
// 	HICON hIcon= ::AfxGetApp()->LoadIcon(IDI_ICON_SERVERROOT);
// 	cImageList.Add(hIcon);
// 	m_Tree.SetImageList(&cImageList, TVSIL_NORMAL);
	m_Tree.ModifyStyle(0, TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);
	HTREEITEM root = m_Tree.InsertItem(_T("server"));

	// 记录日志显示属性
	m_logWindow.SetLimitText(500000);

	// 初始化设置窗口
	settingDlg = new ServerSetting(this);
	settingDlg->Create(IDD_SERVERSETTING_DIALOG);

	ServerParam::instance->mainFrame = this->GetSafeHwnd();

	// 开启后自动开启服务器
	Server::getInstance()->start();
	// 最小化托盘
	::PostMessage(ServerParam::instance->mainFrame, WM_HIDETASK, 0, 0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCardReaderServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if(nID==SC_MINIMIZE) {
		this->ToTray();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCardReaderServerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCardReaderServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// TODO: 开始按钮的点击响应
void CCardReaderServerDlg::OnButtonStart() 
{
	// TODO: Add your control notification handler code here
	if (FALSE == Server::getInstance()->status)
	{
		if (Server::getInstance()->start() != 0)
		{
			AfxMessageBox("开启失败");
		}
	} else {
		AfxMessageBox("服务器已开启");
	}
	
}

// TODO: 停止按钮的点击响应
void CCardReaderServerDlg::OnButtonStop() 
{
	// TODO: Add your control notification handler code here
	if (TRUE == Server::getInstance()->status)
	{
		if (Server::getInstance()->stop() != 0)
		{
			AfxMessageBox("关闭失败");
		}
	} else {
		AfxMessageBox("服务器未开启");
	}
}

// TODO: 重启按钮的点击响应
void CCardReaderServerDlg::OnButtonRestart() 
{
	// TODO: Add your control notification handler code here
	if (TRUE == Server::getInstance()->status) 
	{
		if (Server::getInstance()->restart() != 0)
		{
			AfxMessageBox("重启失败");
		}
	} else {
		AfxMessageBox("服务器未开启");
	}
	
}

// TODO: 单击设置按钮
void CCardReaderServerDlg::OnButtonSetting() 
{
	// TODO: Add your control notification handler code here
	
	settingDlg->ShowWindow(SW_SHOWNORMAL);
}

// TODO: 单击清空按钮
void CCardReaderServerDlg::OnButtonClear() 
{
	// TODO: Add your control notification handler code here
	m_logWindow.SetWindowText(""); // 清空日志窗口
}

// 日志button响应
void CCardReaderServerDlg::OnButtonLog() 
{
	// TODO: Add your control notification handler code here
	WinExec(CString("notepad ") + SimpleLog::GetlogFileLocation(), SW_SHOWNORMAL); // 在记事本里打开日志文件
}

LRESULT CCardReaderServerDlg::UpdateLog(WPARAM wparam,LPARAM lparam)
{
	if (this->m_logWindow.GetLineCount() >= 1000) // 超过1000行,清空一次
	{
		this->m_logWindow.SetWindowText("");
	}
	int len = this->m_logWindow.GetWindowTextLength();
	this->m_logWindow.SetSel(len,len);
	this->m_logWindow.ReplaceSel(Server::getInstance()->log);

	WaitForSingleObject(SimpleLog::getMutex(), 100);
	//EnterCriticalSection(&(Server::getInstance()->g_cs));
	Server::getInstance()->log = "";
	Server::getInstance()->log.Empty();// 清空日志
	//LeaveCriticalSection(&(Server::getInstance()->g_cs));
	ReleaseMutex(SimpleLog::getMutex());
	
	// 动态显示客户端节点
	m_Tree.DeleteAllItems();
	HTREEITEM root = m_Tree.InsertItem(_T("server"));

	// 添加所有读卡器
	map<int, HTREEITEM> readersTree;
	for (map<int, int>::iterator i = ServerParam::instance->readers.begin();
		i != ServerParam::instance->readers.end(); ++i)
	{
		char name[512];
		sprintf(name, "读卡器 %d", i->first);
		readersTree[i->first] = m_Tree.InsertItem(name, root);
	}
	EnterCriticalSection(&(Server::getInstance()->clients_cs));
	// 添加所有客户端到对应读卡器节点
// 	for (list<Client*>::iterator iter = Server::getInstance()->clients.begin();
// 		iter != Server::getInstance()->clients.end(); ++iter)
// 	{
// 		char name[512];
// 		(*iter)->getName(name);
// 		// TODO：引起崩溃的一个地方，getReaderId函数
// 		m_Tree.InsertItem(name, readersTree[(*iter)->getReaderId()]);
// 	}
	for (map<int, list<Client*> >::iterator iter = Server::getInstance()->waitList.begin();
		iter != Server::getInstance()->waitList.end(); ++iter)
	{
		for (list<Client*>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
		{
			char name[512];
			(*iter2)->getName(name);
			m_Tree.InsertItem(name, readersTree[iter->first]);
		}
	}
	LeaveCriticalSection(&(Server::getInstance()->clients_cs));	

	// 展开所有节点, 在添加所有节点以后再展开
	m_Tree.Expand(root, TVE_EXPAND);
	for (map<int, HTREEITEM>::iterator miter = readersTree.begin();
		miter != readersTree.end(); ++miter)
	{
		m_Tree.Expand(miter->second, TVE_EXPAND);
	}

	// 更新显示统计信息
	UpdateData(TRUE);
	m_errors = SimpleLog::getErrors();
	m_warns = SimpleLog::getWarns();
	UpdateData(FALSE);

	return 0;
}

BOOL CCardReaderServerDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	return CDialog::DestroyWindow();
}

void CCardReaderServerDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	// TODO: Add your message handler code here

	// 释放内存, 保证内存不会泄漏
	delete settingDlg;
	delete ServerParam::instance;
	delete Server::getInstance();
}

void CCardReaderServerDlg::ToTray()
{
	NOTIFYICONDATA nid; 
    nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA); 
    nid.hWnd=this->m_hWnd; 
    nid.uID=IDR_MAINFRAME; 
    nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; 
    nid.uCallbackMessage=WM_SHOWTASK;//自定义的消息名称 
    nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)); 
 //   strcpy(nid.szTip, "密钥保安站");    //信息提示条
    Shell_NotifyIcon(NIM_ADD,&nid);    //在托盘区添加图标 

//	nid.uTimeout = 1000;
//	nid.uVersion=NOTIFYICON_VERSION;
//	strcpy(nid.szInfoTitle,_T("密钥保安站"));
//	strcpy(nid.szInfo,_T("密钥保安站最小化到托盘了!"));
	Shell_NotifyIcon(NIM_MODIFY,&nid);

    ShowWindow(SW_HIDE);    //隐藏主窗口
}

LRESULT CCardReaderServerDlg::OnShowTask(WPARAM wParam,LPARAM lParam)
{
	if(wParam!=IDR_MAINFRAME) 
        return 1; 
    switch(lParam) 
    {    
	case WM_RBUTTONUP://右键起来时弹出快捷菜单，这里只有一个“关闭” 
        { 
			
            LPPOINT lpoint=new tagPOINT; 
            ::GetCursorPos(lpoint);//得到鼠标位置 
            CMenu menu; 
            menu.CreatePopupMenu();//声明一个弹出式菜单 
            //增加菜单项“关闭”，点击则发送消息WM_DESTROY给主窗口（已 
            //隐藏），将程序结束。 
            menu.AppendMenu(MF_STRING,WM_DESTROY,"关闭"); 
            //确定弹出式菜单的位置 
            menu.TrackPopupMenu(TPM_LEFTALIGN,lpoint->x,lpoint->y,this); 
            //资源回收 
            HMENU hmenu=menu.Detach(); 
            menu.DestroyMenu(); 
            delete lpoint; 
        }
		break; 
	case WM_LBUTTONDBLCLK://双击左键的处理 
        { 
            this->ShowWindow(SW_SHOW);//简单的显示主窗口完事儿
			this->SetForegroundWindow();
            DeleteTray();
        } 
		break; 
	default:
		break;
    } 
    return 0; 
}

void CCardReaderServerDlg::DeleteTray()
{
	NOTIFYICONDATA nid; 
	nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA); 
	nid.hWnd=this->m_hWnd; 
	nid.uID=IDR_MAINFRAME; 
	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP ; 
	nid.uCallbackMessage=WM_SHOWTASK;//自定义的消息名称 
	nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)); 
	strcpy(nid.szTip, "密钥保安站");    //信息提示条为“计划任务提醒” 
	Shell_NotifyIcon(NIM_DELETE,&nid);    //在托盘区删除图标 
}
