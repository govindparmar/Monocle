#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

extern "C"
{
	// TODO: Move GDI+ Initialization/Uninitialization somewhere else, call init on IDD_VIEWLOG open and uninit on program exit
	HBITMAP WINAPI HBITMAPFromJpegFile(WCHAR *wszFileName)
	{
		HBITMAP hBmp;
		ULONG_PTR ulToken;
		UINT uHeight = 288, uWidth = 446;
		Gdiplus::GdiplusStartupInput si;
		
		si.DebugEventCallback = NULL;
		si.SuppressBackgroundThread = FALSE;
		si.SuppressExternalCodecs = TRUE;
		si.GdiplusVersion = 1;


		GdiplusStartup(&ulToken, &si, NULL);
		Gdiplus::Bitmap bmp(wszFileName, FALSE);
		
		Gdiplus::Bitmap bmp2(uWidth, uHeight, bmp.GetPixelFormat());
		Gdiplus::Graphics gfx(&bmp2);
		
		gfx.DrawImage(&bmp, 0, 0, uWidth, uHeight);
		bmp2.GetHBITMAP(NULL, &hBmp);
		//GdiplusShutdown(ulToken);

		return hBmp;
	}
}