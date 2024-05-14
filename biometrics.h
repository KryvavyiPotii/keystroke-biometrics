#include <Windows.h>
#include <string>
#include <sstream>

// Window classes.
#define ID_CLASS        L"Identification class"
#define PROFILE_CLASS   L"Profile class"
#define LEARN_CLASS     L"Learning class"
#define AUTH_CLASS      L"Authentication class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define TEXT_WIDTH      150
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define BUTTON_WIDTH    ELEMENT_OFFSET * 6
#define ID_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define ID_HEIGHT	    ELEMENT_OFFSET * 13

// Window identifiers.
#define ID_LOGIN        10
#define ID_USERNAME     11

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int addIdControls(HWND hwndId);
void showError(const std::wstring& cstrError);
void showValue(const std::wstring& cstrName, auto aValue);