#include <stdio.h>
#include <Windows.h>

union Test {
	unsigned long a;
	unsigned char b[4];
};

static int WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hInstPrev, _In_ PSTR cmdline, _In_ int cmdshow) {
	MessageBox(NULL, TEXT("Hello WinAPI Graphical User Interface"), TEXT("Welcome"), 0);
	return 0;
}

static LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	}
}

void main() {
	HINSTANCE hInst;
	ZeroMemory(&hInst, sizeof(HINSTANCE));
	WinMain(&hInst, NULL, "", 0);

	//WNDCLASS wc;
	//wc.lpfnWndProc = WindowProc;
	//wc.lpfnWndProc = WindowProc;
}