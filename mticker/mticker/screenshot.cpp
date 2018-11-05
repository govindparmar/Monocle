#include <Windows.h>
#include <wincodec.h>
#include <comdef.h>
#include <strsafe.h>

#pragma comment(lib, "windowscodecs.lib")
#ifdef __cplusplus
extern "C"
{
#endif
	HRESULT WINAPI SaveJPEG(HBITMAP hBitmap, LPCWSTR lpFileName)
	{
		HRESULT hr = (HRESULT)0L;
		IWICImagingFactory *pFactory = nullptr;
		IWICBitmap *pBitmap = nullptr;
		IWICStream *pStream = nullptr;
		IWICBitmapEncoder *pEncoder = nullptr;
		IWICBitmapFrameEncode *pFrameEncode = nullptr;
		UINT uWidth, uHeight;
		const WICPixelFormatGUID wpFormat = GUID_WICPixelFormat24bppRGB;

		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
		if(FAILED(hr))
		{
			goto cleanup;
		}

		pFactory->CreateBitmapFromHBITMAP(hBitmap, nullptr, WICBitmapIgnoreAlpha, &pBitmap);
		pFactory->CreateStream(&pStream);
		pFactory->CreateEncoder(GUID_ContainerFormatJpeg, &GUID_VendorMicrosoftBuiltIn, &pEncoder);

		pStream->InitializeFromFilename(lpFileName, GENERIC_WRITE | GENERIC_READ);

		pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
		pEncoder->CreateNewFrame(&pFrameEncode, nullptr);

		pBitmap->GetSize(&uWidth, &uHeight);

		pFrameEncode->Initialize(nullptr);
		pFrameEncode->SetSize(uWidth, uHeight);
		pFrameEncode->SetPixelFormat(const_cast<WICPixelFormatGUID *>(&wpFormat));
		pFrameEncode->WriteSource(pBitmap, nullptr);

		pFrameEncode->Commit();
		pEncoder->Commit();

		hr = S_OK;


		cleanup:
		if(pFrameEncode)
			pFrameEncode->Release();
		if(pEncoder)
			pEncoder->Release();
		if(pStream)
			pStream->Release();
		if(pBitmap)
			pBitmap->Release();
		if(pFactory)
			pFactory->Release();

		pFrameEncode = nullptr;
		pEncoder = nullptr;
		pStream = nullptr;
		pBitmap = nullptr;
		pFactory = nullptr;

		CoUninitialize();
		return hr;
	}

	BOOL WINAPI SaveBitmap(WCHAR *wPath, SYSTEMTIME st)
	{
		BITMAPFILEHEADER bfHeader;
		BITMAPINFOHEADER biHeader;
		BITMAPINFO bInfo;
		HGDIOBJ hTempBitmap;
		HBITMAP hBitmap;
		BITMAP bAllDesktops;
		HDC hDC, hMemDC;
		LONG lWidth, lHeight;
		BYTE *bBits = NULL;
		HANDLE hHeap = GetProcessHeap();
		DWORD cbBits, dwWritten = 0;
		WCHAR wJpgPath[MAX_PATH];

		ZeroMemory(&bfHeader, sizeof(BITMAPFILEHEADER));
		ZeroMemory(&biHeader, sizeof(BITMAPINFOHEADER));
		ZeroMemory(&bInfo, sizeof(BITMAPINFO));
		ZeroMemory(&bAllDesktops, sizeof(BITMAP));

		hDC = GetDC(NULL);
		hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
		GetObjectW(hTempBitmap, sizeof(BITMAP), &bAllDesktops);

		lWidth = bAllDesktops.bmWidth;
		lHeight = bAllDesktops.bmHeight;

		DeleteObject(hTempBitmap);

		bfHeader.bfType = (WORD)('B' | ('M' << 8));
		bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		biHeader.biSize = sizeof(BITMAPINFOHEADER);
		biHeader.biBitCount = 24;
		biHeader.biCompression = BI_RGB;
		biHeader.biPlanes = 1;
		biHeader.biWidth = lWidth;
		biHeader.biHeight = lHeight;

		bInfo.bmiHeader = biHeader;

		cbBits = (((24 * lWidth + 31)&~31) / 8) * lHeight;
		bBits = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBits);
		if(bBits == NULL)
		{
			MessageBoxW(NULL, L"Out of memory", L"Error", MB_OK | MB_ICONSTOP);
			return FALSE;
		}

		hMemDC = CreateCompatibleDC(hDC);
		hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID **)&bBits, NULL, 0);
		SelectObject(hMemDC, hBitmap);
		BitBlt(hMemDC, 0, 0, lWidth, lHeight, hDC, 0, 0, SRCCOPY);

		/*StringCchPrintfW(wJpgPath, MAX_PATH, L"C:\\Temp\\%.4hu-%.2hu-%.2hu-%.2hu-%.2hu-%.2hu.jpeg", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		hFile = CreateFileW(wJpgPath, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
		WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
		WriteFile(hFile, bBits, cbBits, &dwWritten, NULL);

		CloseHandle(hFile);*/
		StringCchPrintfW(wJpgPath, MAX_PATH, L"%s\\%.4hu-%.2hu-%.2hu-%.2hu-%.2hu-%.2hu.jpg", wPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		Sleep(100);
		SaveJPEG(hBitmap, wJpgPath);
		Sleep(100);
		HeapFree(hHeap, 0, bBits);
		bBits = NULL;
		DeleteDC(hMemDC);
		ReleaseDC(NULL, hDC);
		DeleteObject(hBitmap);

		return TRUE;
	}


#ifdef __cplusplus
}
#endif