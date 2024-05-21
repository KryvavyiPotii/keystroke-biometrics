#include <Windows.h>
#include <commctrl.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Include statistics data.
#include "student.h"

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
#define FORM_LOGIN        20
#define FORM_REGISTER     21
#define FORM_USERNAME     22
#define FORM_PASSWORD     23
#define FORM_SUBMIT       24
#define FORM_CANCEL       25

// Message identifiers.
#define WM_SENDKEYSTROKEDATA    (WM_USER + 0x0001)
#define WM_FORMTYPE             (WM_USER + 0x0002)
#define WM_LOGIN                (WM_USER + 0x0003)
#define WM_REGISTER             (WM_USER + 0x0004)

// Identifiers for WM_COPYDATA communication between windows.
#define SRID_PRESS  1
#define SRID_HOLD   2

// Security.
#define CREDS_DELIMITER     L"$"
#define USERNAME_SIZE       32
#define PASSWORD_SIZE       12

// Path to file with user credentials.
const wchar_t cstrCredsPath[] = L"./creds.txt";
// List of banned characters for isValid function.
const wchar_t wcBanned[] = { L'%', CREDS_DELIMITER[0] };

// Debugging.
// Show error message box.
// Arguments:
//     [in] strError - wstring with error message.
void showError(const std::wstring& strError)
{
    std::wstringstream sstr;

    sstr << strError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring str = sstr.str();

    MessageBox(NULL, str.c_str(), L"Error", MB_OK);
}
// Show value of some numeric variable.
// Arguments:
//     [in] strName - variable name.
//         However, it can be any message that developer finds comfortable enough.
//     [in] aValue - numeric value that needs to be shown.
void showValue(const std::wstring& strName, auto aValue)
{
    std::wstringstream sstrValue;

    sstrValue << aValue << std::endl;

    std::wstring strValue = sstrValue.str();

    MessageBox(NULL, strValue.c_str(), strName.c_str(), 0);
}
void DBG_dwShowArr(const DWORD* dwArr, int iSize)
{
    std::wstring out;

    for (int i = 0; i < iSize; i++)
    {
        out.append(L"\t");
        out.append(std::to_wstring(dwArr[i]));
    }

    MessageBox(0, out.c_str(), L"Out", 0);
}

// Auxiliary.
// Convert UTF-8 string to wide character string.
// Arguments:
//     [in, out] strWide - reference to wstring that receives converted data.
//     [in] strMultibyte - reference to string with UTF-8 data.
// Return value:
//     Success - 0.
//     Failure - -1.
int toWide(std::wstring& strWide, const std::string& strMultibyte)
{
    // Get necessary buffer size.
    int iSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        &strMultibyte[0],
        -1,
        NULL,
        0
    );

    if (!iSize)
    {
        showError(L"toWide::SIZE::MultiByteToWideChar");
        return -1;
    }

    // Convert UTF-8 (multibyte) characters to wide characters.
    strWide.resize(iSize);

    if (!MultiByteToWideChar(
        CP_UTF8,
        0,
        &strMultibyte[0],
        -1,
        &strWide[0],
        iSize))
    {
        showError(L"toWide::CONVERSION::MultiByteToWideChar");
        return -1;
    }

    return 0;
}
// Convert wide character string to UTF-8 string.
// Arguments:
//     [in, out] strMultibyte - reference to string that receives converted data.
//     [in] strWide - reference to wstring.
// Return value:
//     Success - 0.
//     Failure - -1.
int toMultibyte(std::string& strMultibyte, const std::wstring& strWide)
{
    // Get necessary buffer size.
    int iSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        &strWide[0],
        -1,
        NULL,
        0,
        NULL,
        NULL
    );
    if (!iSize)
    {
        showError(L"toMultibyte::SIZE::WideCharToMultiByte");
        return -1;
    }

    // Convert wide characters to UTF-8 (multibyte) characters.
    strMultibyte.resize(iSize);

    if (!WideCharToMultiByte(
        CP_UTF8,
        0,
        &strWide[0],
        -1,
        &strMultibyte[0],
        iSize,
        NULL,
        NULL))
    {
        showError(L"toMultibyte::CONVERSION::WideCharToMultiByte");
        return -1;
    }

    return 0;
}

// GUI functions.
// Main window procedure.
LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Form window procedure.
LRESULT CALLBACK FormProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Keystroke biometrics window procedure.
LRESULT CALLBACK KeystrokeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
HWND createForm(HWND hwndOwner, int iType)
{
    // Create user window.
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    if (!hInstance)
    {
        showError(L"createForm::GetWindowLongW");
        return NULL;
    }

    // Set form title.
    std::wstring strTitle;

    if (iType == FORM_LOGIN)
    {
        strTitle = L"Login";
    }
    else if (iType == FORM_REGISTER)
    {
        strTitle = L"Register";
    }
    else
    {
        showError(L"createForm::INCORRECT_TYPE");
        return NULL;
    }

    // Create and show user window.
    HWND hwndForm = CreateWindow(
        FORM_CLASS, strTitle.c_str(),
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        FORM_WIDTH, FORM_HEIGHT,
        hwndOwner, NULL, hInstance, NULL
    );

    if (hwndForm)
    {
        ShowWindow(hwndForm, SW_SHOW);
        UpdateWindow(hwndForm);
    }
    else
    {
        showError(L"createForm::CreateWindow");
        return NULL;
    }

    return hwndForm;
}
// Add controls to the main window.
// Arguments:
//     [in] hwndMain - handle to the main window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMainControls(HWND hwndMain)
{
    if (!CreateWindow(
        L"BUTTON", L"Login",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (MAIN_WIDTH) / 2 - BUTTON_WIDTH - ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        BUTTON_WIDTH * 2,
        ELEMENT_HEIGHT,
        hwndMain, (HMENU)MAIN_LOGIN, NULL, NULL
    ))
    {
        showError(L"addMainControls::BUTTON::LOGIN");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Register",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (MAIN_WIDTH) / 2 - BUTTON_WIDTH - ELEMENT_OFFSET,
        ELEMENT_OFFSET * 4,
        BUTTON_WIDTH * 2,
        ELEMENT_HEIGHT,
        hwndMain, (HMENU)MAIN_REGISTER, NULL, NULL
    ))
    {
        showError(L"addMainControls::BUTTON::LOGIN");
        return -1;
    }

    return 0;
}
int addFormControls(HWND hwndForm)
{
    if (!CreateWindow(
        L"STATIC", L"Username:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, NULL, NULL, NULL
    ))
    {
        showError(L"addFormControls::STATIC::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_USERNAME, NULL, NULL
    ))
    {
        showError(L"addFormControls::EDIT::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 5,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, NULL, NULL, NULL
    ))
    {
        showError(L"addFormControls::STATIC::PASSWORD");
        return -1;
    }

    HWND hwndPassword = CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 7,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_PASSWORD, NULL, NULL
    );

    if (!hwndPassword)
    {
        showError(L"addFormControls::EDIT::PASSWORD");
        return -1;
    }

    // Subclass the password EDIT control to gather keystroke data.
    SetWindowSubclass(hwndPassword, &KeystrokeProc, 0, 0);

    if (!CreateWindow(
        L"BUTTON", L"Submit",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        FORM_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_SUBMIT, NULL, NULL
    ))
    {
        showError(L"addFormControls::BUTTON::SUBMIT");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        FORM_WIDTH - BUTTON_WIDTH - ELEMENT_OFFSET * 3,
        FORM_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_CANCEL, NULL, NULL
    ))
    {
        showError(L"addFormControls::BUTTON::CANCEL");
        return -1;
    }

    return 0;
}
void sendData(HWND hwndReceiver, HWND hwndSender, const DWORD* pdwData, DWORD dwSize, DWORD dwID)
{
    // Create struct to contain data.
    COPYDATASTRUCT cds{};

    cds.dwData = dwID;
    cds.cbData = dwSize;
    cds.lpData = (DWORD*)pdwData;

    // Send data to target.
    SendMessage(
        hwndReceiver,
        WM_COPYDATA,
        (WPARAM)hwndSender,
        (LPARAM)(LPVOID)&cds
    );
}

// Statistics.
double expectation(const DWORD* pdwArr, DWORD dwSize)
{
    double exp = 0;

    for (DWORD i = 0; i < dwSize; i++)
    {
        exp += pdwArr[i];
    }

    exp /= dwSize;

    return exp;
}
double variance(const DWORD* pdwArr, DWORD dwSize)
{
    double var = 0;

    double exp = expectation(pdwArr, dwSize);

    for (DWORD i = 0; i < dwSize; i++)
    {
        var += (pdwArr[i] - exp) * (pdwArr[i] - exp);
    }

    var /= dwSize;

    return var;
}
int excludeErrors(std::vector<DWORD>* pvResult, const DWORD* pdwArr, DWORD dwSize)
{
    return 0;
}
int compareData(double dCredsVariance, double dAuthVariance)
{
    return 1;
}


// Credential processing.
// Check entered credentials for banned characters.
// Arguments:
//     [in] strInput - reference to string with credentials.
// Return value:
//     Success - 1.
//     Failure - 0 (user not found), -1 (error).
int isValid(const std::wstring& strInput)
{
    // Check banned characters.
    for (int i = 0; i < wcslen(wcBanned); i++)
    {
        if (strInput.find(wcBanned[i]) != std::wstring::npos)
        {
            return 0;
        }
    }

    return 1;
}
// Read contents of credentials file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in, out] strCreds - reference to wstring that receives credentials.
// Return value:
//     Success - 0.
//     Failure - -1.
int readCreds(std::wstring& strCreds)
{
    // Open file for reading.
    std::ifstream credsFile(cstrCredsPath, std::ios::binary);

    if (!credsFile.is_open())
    {
        showError(L"readCreds::std::ifstream::is_open");
        return -1;
    }

    // Read file contents
    std::string strMultibyteCreds;

    strMultibyteCreds.assign(
        (std::istreambuf_iterator<char>(credsFile)),
        (std::istreambuf_iterator<char>())
    );

    // Cleanup.
    credsFile.close();

    // Convert multibyte data to wide characters.
    if (toWide(strCreds, strMultibyteCreds) < 0)
    {
        return -1;
    }

    return 0;
}
// Class that represents a user with their credentials.
class User
{
public:
    User() {}

    // Methods to retrieve user credentials.
    std::wstring getUsername() { return strUsername; }
    std::wstring getPassword() { return strPassword; }
    double getPressVar() { return dPressVariance; }
    double getHoldVar() { return dHoldVariance; }

    // Methods to set user credentials.
    void setUsername(const std::wstring strNewUsername)
    {
        strUsername = strNewUsername;
    }
    void setPassword(const std::wstring strNewPassword)
    {
        strPassword = strNewPassword;
    }
    void setPressVar(double dNewPressVariance)
    {
        dPressVariance = dNewPressVariance;
    }
    void setHoldVar(double dNewHoldVariance)
    {
        dHoldVariance = dNewHoldVariance;
    }

    // Fill object's parameters based on the credentials string.
    int getCreds(const std::wstring& strCreds)
    {
        // Find position and length of user entry.
        std::vector<int> vIndices = findEntry(strCreds);

        if (vIndices[0] == -1 && vIndices[1] == -1)
        {
            // If the entry isn't found, user isn't registered.
            return -1;
        }

        // Store the entry.
        std::wstring strEntry;

        strEntry = strCreds.substr(vIndices[0], vIndices[1]); // do not include line feed

        // Parse entry and fill User object.
        if (this->parseEntry(strCreds) < 0)
        {
            return -1;
        }

        return 0;

        return 0;
    }

    // Check if user is registered by checking the credentials string.
    int isRegistered(const std::wstring& strCreds)
    {
        // Find position and length of user entry.
        std::vector<int> vIndices = findEntry(strCreds);

        if (vIndices[0] == -1 && vIndices[1] == -1)
        {
            // If the entry isn't found, user isn't registered.
            return 0;
        }

        return 1;
    }

    // Find position and length of user credentials entry.
    // Arguments:
    //     [in] strCreds - referense to wstring with all credentials.
    // Return value:
    //     Success - 2D vector { position of entry; length of entry }.
    //     Failure - 2D vector { -1; -1 }.
    std::vector<int> findEntry(const std::wstring& strCreds)
    {
        // Check if username is set.
        if (strUsername.empty())
        {
            return { -1, -1 };
        }

        // We need to append delimiter to avoid deleting wrong user.
        // Example: trying to delete user "bob" we may accidently delete "bob123".
        std::wstring str = strUsername + CREDS_DELIMITER;

        // Get user credentials entry.
        int iEntryPosition = strCreds.find(str.c_str());

        if (iEntryPosition == std::wstring::npos)
        {
            return { -1, -1 };
        }

        // Store location of entry that is to be removed.
        int iEntryLength = 0;

        for (int i = iEntryPosition; i < strCreds.size() + 1; i++)
        {
            if (strCreds[i] == 0 || strCreds[i] == L'\n')
            {
                iEntryLength = i - iEntryPosition;
                break;
            }
        }

        return { iEntryPosition, iEntryLength };
    }

    // Parse an entry with credentials and fill object parameters.
    int parseEntry(const std::wstring& strEntry)
    {
        User uTemp = *this;
        std::wstring strTemp(strEntry.c_str());
        int iBeginIndex;
        int iDelimiterIndex;

        // Get username from entry.
        iBeginIndex = 0;
        iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
        this->setUsername(strTemp.substr(0, iDelimiterIndex));

        if (strUsername.empty())
        {
            showError(L"User::parseEntry()::USERNAME");
            // Undo changes.
            *this = uTemp;
            return -1;
        }

        iBeginIndex = iDelimiterIndex + 1;

        // Get password from entry.
        strTemp = strTemp.substr(iBeginIndex);
        iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
        this->setPassword(strTemp.substr(0, iDelimiterIndex));

        if (strPassword.empty())
        {
            showError(L"User::parseEntry()::PASSWORD");
            *this = uTemp;
            return -1;
        }

        std::wstring wstrNumber;

        iBeginIndex = iDelimiterIndex + 1;
        strTemp = strTemp.substr(iBeginIndex);
        iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
        wstrNumber = strTemp.substr(0, iDelimiterIndex);
        this->setPressVar(stod(wstrNumber));

        if (!dPressVariance)
        {
            showError(L"User::parseEntry()::PRESS_VARIANCE");
            *this = uTemp;
            return -1;
        }

        iBeginIndex = iDelimiterIndex + 1;
        iDelimiterIndex = strTemp.find(L"\n");
        wstrNumber = strTemp.substr(iBeginIndex, iDelimiterIndex);
        this->setHoldVar(stod(wstrNumber));

        if (!dHoldVariance)
        {
            showError(L"User::parseEntry()::HOLD_VARIANCE");
            *this = uTemp;
            return -1;
        }

        return 0;
    }

private:
    std::wstring strUsername;
    std::wstring strPassword;
    double dPressVariance = 0;
    double dHoldVariance = 0;
};
// Identify user by username.
int identify(User* pUser, const std::wstring& strCreds, HWND hwndForm)
{
    // Create buffer for username.
    std::wstring strUsername(USERNAME_SIZE + 1, 0);

    // Get handle to username field.
    HWND hwndUsername = GetDlgItem(hwndForm, FORM_USERNAME);

    if (!hwndUsername)
    {
        showError(L"identify::GetDlgItem");
        return -1;
    }

    // Get username.
    int iSize = GetWindowText(hwndUsername, &strUsername[0], USERNAME_SIZE);

    if (!iSize)
    {
        if (!GetLastError())
        {
            MessageBox(
                NULL,
                L"Username is empty",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }
        else
        {
            showError(L"identify::GetWindowText");
        }

        return -1;
    }

    // Resize buffer to remove extra null bytes.
    strUsername.resize(iSize);

    // Check if username contains banned characters.
    if (!isValid(strUsername))
    {
        MessageBox(
            NULL,
            L"Username contains forbidden characters",
            L"Alert",
            MB_OK | MB_ICONWARNING
        );
        return -1;
    }

    pUser->setUsername(strUsername);

    // Check if user is registered.
    return pUser->isRegistered(strCreds);
}
int authenticate(User* pUser, std::vector<DWORD>* pvPress, std::vector<DWORD>* pvHold, const std::wstring& strCreds, HWND hwndForm)
{
    // Get user password.
    std::wstring strPassword(PASSWORD_SIZE + 1, 0);
    {
        // Get handle to password field.
        HWND hwndPassword = GetDlgItem(hwndForm, FORM_PASSWORD);

        if (hwndPassword == NULL)
        {
            showError(L"authenticate::GetDlgItem");
            return -1;
        }

        // Get password.
        int iSize = GetWindowText(hwndPassword, &strPassword[0], PASSWORD_SIZE);

        // Check if reading didn't fail.
        if (GetLastError())
        {
            return -1;
        }
        // Check if password field is empty.
        if (!iSize)
        {
            return 0;
        }

        // Resize buffer to remove extra null bytes.
        strPassword.resize(iSize);
    }

    pUser->setPassword(strPassword);

    // Get user credentials from credentials file.
    User userCreds;

    userCreds.getCreds(strCreds);

    // Compare passwords.
    if (userCreds.getPassword().compare(pUser->getPassword()) != 0)
    {
        return 0;
    }

    // Process keystroke data.
    double dPressVariance, dHoldVarience;



    // Compare processed data.
    if (compareData(userCreds.getPressVar(), dPressVariance) != 1)
    {
        return 0;
    }

    if (compareData(userCreds.getHoldVar(), dHoldVarience) != 1)
    {
        return 0;
    }

    return 1;
}
