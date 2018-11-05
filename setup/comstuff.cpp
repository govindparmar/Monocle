#include <Windows.h>
#include <WinNls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <ObjIdl.h>
#include <ShlGuid.h>
#include <taskschd.h>
#include <comdef.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

extern "C"
{
	HRESULT WINAPI CreateSchedTask(WCHAR *wszMonitorExePath)
	{
		ITaskService *pService = NULL;
		ITaskFolder *pRoot = NULL;
		ITaskDefinition *pTask = NULL;
		ITaskSettings *pSettings = NULL;
		IRegistrationInfo *pInfo = NULL;
		ITriggerCollection *pCollection = NULL;
		ITrigger *pTrigger = NULL;
		ILogonTrigger *pLogon = NULL;
		IPrincipal *pPrincipal = NULL;
		IActionCollection *pActionCollection = NULL;
		IAction *pAction = NULL;
		IExecAction *pExecAction = NULL;
		IRegisteredTask *pRegTask = NULL;

		VARIANT vBlank = _variant_t();

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
		CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID *)&pService);

		pService->Connect(vBlank, vBlank, vBlank, vBlank);
		pService->GetFolder(SysAllocString(L"\\"), &pRoot);
		pService->NewTask(0, &pTask);
		pService->Release();

		pTask->get_RegistrationInfo(&pInfo);

		pInfo->put_Author(SysAllocString(L"Monocle by Govind Parmar"));

		pInfo->Release();

		pTask->get_Settings(&pSettings);
		pSettings->put_StartWhenAvailable(VARIANT_TRUE);
		pSettings->put_Enabled(VARIANT_TRUE);
		pSettings->Release();

		pTask->get_Triggers(&pCollection);
		pCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
		pCollection->Release();
		pTrigger->QueryInterface(IID_ILogonTrigger, (void **)&pLogon);

		pLogon->put_Id(SysAllocString(L"LogonTrigger"));
		pLogon->Release();

		pTask->get_Actions(&pActionCollection);
		pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
		pActionCollection->Release();

		pAction->QueryInterface(IID_IExecAction, (void **)&pExecAction);
		pAction->Release();

		pExecAction->put_Path(SysAllocString(wszMonitorExePath));
		pExecAction->Release();

		pTask->get_Principal(&pPrincipal);
		pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
		pPrincipal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT);
		pTask->put_Principal(pPrincipal);
		pPrincipal->Release();

		pRoot->RegisterTaskDefinition(
			SysAllocString(L"Monocle Monitor"), 
			pTask, TASK_CREATE_OR_UPDATE, 
			_variant_t(L"NT AUTHORITY\\SYSTEM"), 
			_variant_t(), TASK_LOGON_SERVICE_ACCOUNT, 
			_variant_t(L""), &pRegTask);

		pRoot->Release();
		pTask->Release();
		pRegTask->Release();
		CoUninitialize();

		return S_OK;
	}

	HRESULT WINAPI CreateShortcut(WCHAR *wszFilePath, WCHAR *wszOutLnkPath, WCHAR *wszDesc)
	{
		IShellLinkW *pShellLink = nullptr;
		IPersistFile *pPersistFile = nullptr;
		HRESULT hr;

		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *)&pShellLink);

		pShellLink->SetPath(wszFilePath);
		pShellLink->SetDescription(L"Monocle Log Viewer");

		hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
		if(SUCCEEDED(hr))
		{
			hr = pPersistFile->Save(wszOutLnkPath, TRUE);
			pPersistFile->Release();
		}

		pShellLink->Release();
		CoUninitialize();
		return hr;
	}
}