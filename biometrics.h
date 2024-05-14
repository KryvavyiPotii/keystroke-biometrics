#include <Windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Window classes.
#define MAIN_CLASS   L"Main class"
#define FORM_CLASS   L"User form class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define TEXT_WIDTH      150
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define BUTTON_WIDTH    ELEMENT_OFFSET * 6
#define MAIN_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define MAIN_HEIGHT	    ELEMENT_OFFSET * 11
#define FORM_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define FORM_HEIGHT	    ELEMENT_OFFSET * 17

// Window identifiers.
#define MAIN_LOGIN        10
#define MAIN_REGISTER     11
#define FORM_USERNAME     20
#define FORM_PASSWORD     21
#define FORM_SUBMIT       22
#define FORM_CANCEL       23

// Window titles.
#define FORM_LOGIN      L"Login"
#define FORM_REGISTER   L"Register"

// Security.
#define CREDS_DELIMITER     L"$"
#define USERNAME_SIZE       32
#define PASSWORD_SIZE       64
#define USER_NOTREGISTERED  0
#define USER_REGISTERED     1

// GUI functions.
// Main window procedure.
LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Keystroke biometrics window procedure.
LRESULT CALLBACK FormProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Add controls to the main window.
// Arguments:
//     [in] hwndMain - handle to the main window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMainControls(HWND hwndMain);
int addFormControls(HWND hwndForm);

// Security functions.
// Identify user by username.
// Arguments:
//     [in, out] strUsername - reference to string that receives entered username.
//     [in] hwndForm - handle to a form window.
// Return value:
//     Success - 0.
//     Failure - -1.
int identify(std::wstring& strUsername, HWND hwndForm);
int openForm(HWND hwndOwner, const std::wstring& strFormTitle);
// Check entered credentials for banned characters.
// Arguments:
//     [in] strInput - reference to string with credentials.
// Return value:
//     Success - 1.
//     Failure - 0 (user not found), -1 (error).
int isValid(const std::wstring& strInput);
// Read contents of credentials file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in, out] strCreds - reference to string that receives credentials.
// Return value:
//     Success - 0.
//     Failure - -1.
int readCreds(std::string& strCreds);
// Find position and length of user credentials entry.
// Arguments:
//     [in] strUsername - reference to wstring with username.
//     [in] strCreds - referense to wstring with all credentials.
// Return value:
//     Success - 2D vector { position of entry; length of entry }.
//     Failure - 2D vector { -1; -1 }.
std::vector<int> findEntry(const std::wstring& strUsername, const std::wstring& strCreds);

// Auxiliary functions.
// Convert UTF-8 string to wide character string.
// Arguments:
//     [in, out] strWide - reference to wstring that receives converted data.
//     [in] strMultibyte - reference to string with UTF-8 data.
// Return value:
//     Success - 0.
//     Failure - -1.
int toWide(std::wstring& strWide, const std::string& strMultibyte);
// Convert wide character string to UTF-8 string.
// Arguments:
//     [in, out] strMultibyte - reference to string that receives converted data.
//     [in] strWide - reference to wstring.
// Return value:
//     Success - 0.
//     Failure - -1.
int toMultibyte(std::string& strMultibyte, const std::wstring& strWide);
// Debug functions.
// Show error message box.
// Arguments:
//     [in] strError - wstring with error message.
void showError(const std::wstring& strError);
// Show value of some numeric variable.
// Arguments:
//     [in] strName - variable name.
//         However, it can be any message that developer finds comfortable enough.
//     [in] aValue - numeric value that needs to be shown.
void showValue(const std::wstring& strName, auto aValue);
