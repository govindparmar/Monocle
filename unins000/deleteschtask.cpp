#include <Windows.h>
#include <taskschd.h>
#include <comdef.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

extern "C"
{
	HRESULT WINAPI DeleteMonocleTask()
	{
		ITaskService *pTaskService = nullptr;
		ITaskFolder *pTaskFolder = nullptr;
		VARIANT vBlank = _variant_t();
		HRESULT hr;

		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if(FAILED(hr))
		{
			goto cleanup;
		}
		hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID *)&pTaskService);
		if(FAILED(hr))
		{
			goto cleanup;
		}

		hr = pTaskService->Connect(vBlank, vBlank, vBlank, vBlank);
		if(FAILED(hr))
		{
			goto cleanup;
		}

		hr = pTaskService->GetFolder(SysAllocString(L"\\"), &pTaskFolder);
		if(FAILED(hr))
		{
			goto cleanup;
		}

		hr = pTaskService->Release();
		if(FAILED(hr))
		{
			goto cleanup;
		}

		pTaskService = nullptr;

		hr = pTaskFolder->DeleteTask(SysAllocString(L"Monocle Monitor"), 0);
		if(FAILED(hr))
		{
			goto cleanup;
		}
		hr = pTaskFolder->Release();
		if(FAILED(hr))
		{
			goto cleanup;
		}

		pTaskFolder = nullptr;

	cleanup:
		if(pTaskService) pTaskService->Release();
		if(pTaskFolder) pTaskFolder->Release();
		CoUninitialize();
	
		return hr;
	}
}