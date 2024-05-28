#include <Windows.h>
#include <commctrl.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Include statistics data.
#include "statistics.h"

// Window classes.
#define MAIN_CLASS   L"Main class"
#define FORM_CLASS   L"User form class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define TEXT_WIDTH      150
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define BUTTON_WIDTH    ELEMENT_OFFSET * 6
#define MAIN_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define MAIN_HEIGHT	    ELEMENT_OFFSET * 13
#define FORM_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define FORM_HEIGHT	    ELEMENT_OFFSET * 19

// Window identifiers.
#define MAIN_LOGIN        10
#define MAIN_REGISTER     11
#define MAIN_ABOUT        12
#define FORM_LOGIN        20
#define FORM_REGISTER     21
#define FORM_USERNAME     22
#define FORM_PASSWORD     23
#define FORM_SUBMIT       24
#define FORM_CANCEL       25
#define FORM_ABOUT        26

// Message identifiers.
#define WM_SENDKEYSTROKEDATA    (WM_USER + 0x0001)
#define WM_FORMTYPE             (WM_USER + 0x0002)
#define WM_LOGIN                (WM_USER + 0x0003)
#define WM_REGISTER             (WM_USER + 0x0004)

// Identifiers for WM_COPYDATA communication between windows.
#define SRID_PRESS  1
#define SRID_HOLD   2

// Return values of EDIT control reading functions.
#define READ_ERROR     -1
#define READ_EMPTY     1
#define READ_TOOSHORT  2
#define READ_FORBIDDEN 3

// Security.
#define CREDS_DELIMITER     L"$"
#define USERNAME_MAXSIZE    32
#define PASSWORD_MAXSIZE    12
#define PASSWORD_MINSIZE    6

// Path to file with user credentials.
const wchar_t cstrCredsPath[] = L"./creds.txt";
// List of banned characters for valid function.
const wchar_t cstrBanned[] = { L'%', CREDS_DELIMITER[0] };
// Help message string of the main window.
std::wstring strMainAbout = L"Author: KryvavyiPotii\n\n"
    L"    It's a Win API C++ security program with implemented keystroke biometrics.\n"
    L"    The user can register a new account or login into an existing one.\n"
    L"    Regardless of chosen option they need to specify a username with a password.\n"
    L"    During entering the password, keystroke data is gathered and used as the second authentication factor.";
// Help message string of the form.
std::wstring strFormAbout = L"Input requirements:\n* Username maximum length is "
    + std::to_wstring(USERNAME_MAXSIZE) + L".\n"
    + L"* Password minimum length is " + std::to_wstring(PASSWORD_MINSIZE)
    + L" and maximum length is " + std::to_wstring(PASSWORD_MAXSIZE) + L".\n"
    L"* Input should contain only letters and digits.\n"
    L"    Try to write password as naturally as it is possible and avoid making mistakes.\n"
    L"    In case you mistyped a character, press BACKSPACE or DELETE to clean the whole box and delete gathered data.\n"
    L"    In order to quickly submit the form you can press ENTER while password box is active.";

// Struct that contains data about a key press.
struct KeyPress
{
    WORD wKeyCode = 0;  // virtual-key code
    DWORD dwTime = 0; // time in ms when key was pressed
};

// Struct that contains statistical data.
struct Stats
{
    int iSize = 0;  // n - size of data set.
    double dExpectation = 0;
    double dVariance = 0;
};

// Class that represents a user with their credentials.
class User
{
public:
    User() {}

    // Methods to retrieve user credentials.
    // Return value: strUsername
    std::wstring getUsername();
    // Return value: strPassword
    std::wstring getPassword();
    // Return value: statsPress
    Stats getPressStats();
    // Return value: statsHold
    Stats getHoldStats();

    // Methods to set user credentials.
    // Arguments:
    //     [in] strUsername - reference to a wstring with a new username.
    void setUsername(const std::wstring& strUsername);
    // Arguments:
    //     [in] strPassword - reference to a wstring with a new password.
    void setPassword(const std::wstring& strPassword);
    // Arguments:
    //     [in] statsPress - reference to new key press statistics.
    void setPressStats(const Stats& statsPress);
    // Arguments:
    //     [in] vPressData - reference to a vector with key press data.
    void setPressStats(const std::vector<DWORD>& vPressData);
    // Arguments:
    //     [in] statsHold - reference to new key hold statistics.
    void setHoldStats(const Stats& statsHold);
    // Arguments:
    //     [in] vPressData - reference to a vector with key hold data.
    void setHoldStats(const std::vector<DWORD>& vHoldData);

    // Fill user's parameters based on the credentials string.
    // Arguments:
    //     [in] strUsername - reference to a wstring with a name of the required user.
    //     [in] strCreds - reference to a wstring with all credentials.
    // Return value:
    //     Success - 0.
    //     Failure - -1.
    int getCreds(const std::wstring& strUsername, const std::wstring& strCreds);
    // Add user's entry to the credentials string or update it.
    // Arguments:
    //     [in] strCreds - reference to a wstring with all credentials.
    // Return value:
    //     Success - 0.
    //     Failure - -1.
    int setCreds(std::wstring& strCreds);

    // Check if user is registered.
    // Arguments:
    //     [in] strCreds - reference to a string with all credentials.
    // Return value:
    //     User is registered - 1.
    //     User isn't registered - 0.
    //     Failure - -1.
    int registered(const std::wstring& strCreds);

    // Find position and length of user credentials entry.
    // Arguments:
    //     [in] strCreds - referense to wstring with all credentials.
    // Return value:
    //     Success - 2D vector { position of entry; length of entry }.
    //     Failure - 2D vector { -1; -1 }.
    std::vector<int> findEntry(const std::wstring& strCreds);
    // Create an entry from user parameters.
    // Return value: wstring entry.
    std::wstring createEntry();
    // Parse an entry with credentials and fill user parameters.
    // Arguments:
    //     [in] strEntry - reference to a wstring with an entry.
    // Return value:
    //     Success - 0.
    //     Failure - -1.
    int parseEntry(const std::wstring& strEntry);

private:
    std::wstring strUsername;
    std::wstring strPassword;
    Stats statsPress = { 0, 0, 0 };
    Stats statsHold = { 0, 0, 0 };
};

// GUI.
// Main window procedure.
LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Form window procedure.
LRESULT CALLBACK FormProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Keystroke biometrics window procedure.
LRESULT CALLBACK KeystrokeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
// Create login/registration form window.
// Arguments:
//     [in] hwndOwner - handle to the main window.
//     [in] iType - type of the form (FORM_LOGIN or FORM_REGISTER).
// Return value:
//     Success - handle to the created form window.
//     Failure - NULL pointer.
HWND createForm(HWND hwndOwner, int iType);
// Add controls to the main window.
// Arguments:
//     [in] hwndMain - handle to the main window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMainControls(HWND hwndMain);
// Add a menu to the main window.
// Arguments:
//     [in] hwndMain - handle to the main window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMainMenu(HWND hwndMain);
// Add controls to the form window.
// Arguments:
//     [in] hwndForm - handle to the form window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addFormControls(HWND hwndForm);
// Add a menu to the form window.
// Arguments:
//     [in] hwndForm - handle to the form window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addFormMenu(HWND hwndForm);
// Send data to another window.
// This function is used for sending keystroke data from password EDIT control to the form window.
// Arguments:
//     [in] hwndReceiver - handle to a receiver window.
//     [in] hwndSender - handle to a sender window.
//     [in] vData - reference to a vector with keystoke data of one type.
//     [in] dwID - identifier of the data that is being sent (SRID_PRESS or SRID_HOLD).
void sendData(HWND hwndReceiver, HWND hwndSender, const std::vector<DWORD>& vData, DWORD dwID);
// Clear text in EDIT control of a window/dialog and set keyboard focus to it.
// Arguments:
//     [in] hwndParent - handle to a parent window/dialog.
//     [in] iWindowID - identifier of a control.
void clearEditControl(HWND hwndParent, int iWindowID);

// Login user with 2FA:
// 1. Password (something that I know)
// 2. Keystroke biometrics (something that I am)
// Arguments:
//     [in, out] pUser - pointer to a User object.
//               Before function call it must already contain statistical data and a username.
//               After successful authentication password is added to the pointed object.
//     [in, out] strCreds - reference to a wstring with all credentials.
//               After successful authentication user credentials are updated.
//     [in] hwndForm - handle to the form window.
// Return value:
//     User is authenticated - 1.
//     User isn't authenticated - 0.
//     Failure - -1.
int loginUser(User* pUser, std::wstring& strCreds, HWND hwndForm);
// Register a new user.
// Arguments:
//     [in, out] pUser - pointer to a User object.
//               Before function call it must already contain statistical data.
//               After successful authentication username and password are added to the pointed object.
//     [in, out] strCreds - reference to a wstring with all credentials.
//               After successful registration user credentials are added.
//     [in] hwndForm - handle to the form window.
// Return value:
//     Success - 0.
//     Failure - -1.
int registerUser(User* pUser, std::wstring& strCreds, HWND hwndForm);
// Read input for an EDIT control of a form.
// Arguments:
//     [in, out] strInput - reference to a wstring that receives input.
//     [in] hwndForm - handle to the form window.
//     [in] iEditID - windows identifier of the EDIT control (FORM_USERNAME or FORM_PASSWORD).
// Return value:
//     Success - 0.
//     Reading error                   - READ_ERROR.
//     Empty input                     - READ_EMPTY.
//     Too short input                 - READ_TOOSHORT.
//     Input with forbidden characters - READ_FORBIDDEN.
int readInput(std::wstring& strInput, HWND hwndForm, int iEditID);
// Read contents of the credentials file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in, out] strCreds - reference to wstring that receives credentials.
// Return value:
//     Success - 0.
//     Failure - -1.
int readCreds(std::wstring& strCreds);
// Write data to the credentials file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in] strCreds - reference to wstring that receives credentials.
//     [in] mode - mode in which binary file will be opened.
//          In the function it is ORed with std::ios::binary.
// Return value:
//     Success - 0.
//     Failure - -1.
int writeCreds(const std::wstring& strCreds, std::ios::openmode mode);
// Check entered credentials for banned characters.
// Arguments:
//     [in] strInput - reference to a wstring with data.
// Return value:
//     Success - 1.
//     Failure - 0.
int valid(const std::wstring& strInput);
// Convert a UTF-8 string to a wide character string.
// Arguments:
//     [in, out] strWide - reference to a wstring that receives converted data.
//     [in] strMultibyte - reference to a string with UTF-8 data.
// Return value:
//     Success - 0.
//     Failure - -1.
int toWide(std::wstring& strWide, const std::string& strMultibyte);
// Convert a wide character string to a UTF-8 string.
// Arguments:
//     [in, out] strMultibyte - reference to a string that receives converted data.
//     [in] strWide - reference to a wstring.
// Return value:
//     Success - 0.
//     Failure - -1.
int toMultibyte(std::string& strMultibyte, const std::wstring& strWide);

// Statistics.
// Calculate expectation of the passed data.
// Arguments:
//     [in] vData - reference to a vector with data to be processed.
// Return value: expectation.
double expectation(const std::vector<DWORD>& vData);
// Calculate variance of the passed data.
// Arguments:
//     [in] vData - reference to a vector with data to be processed.
// Return value: variance.
double variance(const std::vector<DWORD>& vData);
// Calculate variance of the passed data.
// Arguments:
//     [in] dExp - precalculated expectation of the passed data.
//     [in] vData - reference to a vector with data to be processed.
// Return value: variance.
double variance(double dExp, const std::vector<DWORD>& vData);
// Apply of an algorithm of error exclusion in observed data.
// Arguments:
//     [in, out] vData - reference to a vector with data to be processed.
void excludeErrors(std::vector<DWORD>* vData);
// Create a Stats struct out of the passed data.
// Arguments:
//     [in] vData - reference to a vector with data to be processed.
// Return value: Stats struct.
Stats calculateStats(const std::vector<DWORD>& vData);
// Check homogeneity of two variances and equality of two centers of distribution.
// Arguments:
//     [in] statsIn1 - reference to the first Stats struct.
//     [in] statsIn2 - reference to the second Stats struct.
// Return value:
//     Stats are equal - 1.
//     Stats aren't equal - 0.
int compareStats(const Stats& statsIn1, const Stats& statsIn2);

// Debugging.
// Show error message box.
// Arguments:
//     [in] strError - reference to a wstring with error message.
void showError(const std::wstring& strError);

std::wstring User::getUsername() { return strUsername; }
std::wstring User::getPassword() { return strPassword; }
Stats User::getPressStats() { return statsPress; }
Stats User::getHoldStats() { return statsHold; }
void User::setUsername(const std::wstring& strUsername)
{
    this->strUsername = strUsername;
}
void User::setPassword(const std::wstring& strPassword)
{
    this->strPassword = strPassword;
}
void User::setPressStats(const Stats& statsPress)
{
    this->statsPress = statsPress;
}
void User::setPressStats(const std::vector<DWORD>& vPressData)
{
    // Create and set new key press stats to the user.
    Stats statsPress = calculateStats(vPressData);

    setPressStats(statsPress);
}
void User::setHoldStats(const Stats& statsHold)
{
    this->statsHold = statsHold;
}
void User::setHoldStats(const std::vector<DWORD>& vHoldData)
{
    // Create and set new key hold stats to the user.
    Stats statsHold = calculateStats(vHoldData);

    setHoldStats(statsHold);
}
int User::getCreds(const std::wstring& strUsername, const std::wstring& strCreds)
{
    // Set username.
    setUsername(strUsername);

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
    if (parseEntry(strEntry) < 0)
    {
        return -1;
    }

    return 0;
}
int User::setCreds(std::wstring& strCreds)
{    
    // Create a new entry.
    std::wstring strNewEntry = createEntry();
    
    // If user already has an entry, remove it.
    if (registered(strCreds))
    {
        // Find the old entry
        std::vector<int> vIndices = findEntry(strCreds);

        if (vIndices[0] == -1 && vIndices[1] == -1)
        {
            return -1;
        }

        // Remove the found entry.
        strCreds.erase(vIndices[0], vIndices[1] + 2);
    }

    // Add the created entry to credentials.
    strCreds.append(strNewEntry.c_str());

    return 0;
}
int User::registered(const std::wstring& strCreds)
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
std::vector<int> User::findEntry(const std::wstring& strCreds)
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
        if (strCreds[i] == 0 || strCreds[i] == L'\r')
        {
            iEntryLength = i - iEntryPosition;
            break;
        }
    }

    return { iEntryPosition, iEntryLength };
}
std::wstring User::createEntry()
{
    // Remove extra null bytes.
    strUsername.erase(
        std::find(strUsername.begin(), strUsername.end(), '\0'),
        strUsername.end()
    );

    strPassword.erase(
        std::find(strPassword.begin(), strPassword.end(), '\0'),
        strPassword.end()
    );

    std::wstring strEntry = strUsername + CREDS_DELIMITER
        + strPassword + CREDS_DELIMITER
        + std::to_wstring(statsPress.iSize) + CREDS_DELIMITER
        + std::to_wstring(statsPress.dExpectation) + CREDS_DELIMITER
        + std::to_wstring(statsPress.dVariance) + CREDS_DELIMITER
        + std::to_wstring(statsHold.iSize) + CREDS_DELIMITER
        + std::to_wstring(statsHold.dExpectation) + CREDS_DELIMITER
        + std::to_wstring(statsHold.dVariance) + L"\r\n";

    return strEntry;
}
int User::parseEntry(const std::wstring& strEntry)
{
    std::wstring strTemp(strEntry.c_str());
    int iBeginIndex, iDelimiterIndex;

    // Parse strings.
    std::wstring strUsername, strPassword;

    // Get a username from the entry.
    iBeginIndex = 0;
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strUsername = strTemp.substr(0, iDelimiterIndex);

    if (strUsername.empty())
    {
        showError(L"User::parseEntry()::USERNAME");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get a password.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strPassword = strTemp.substr(0, iDelimiterIndex);

    if (strPassword.empty())
    {
        showError(L"User::parseEntry()::PASSWORD");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Parse double values.
    std::wstring strNumber;
    Stats statsPress, statsHold;

    // Get size of key press data.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strNumber = strTemp.substr(0, iDelimiterIndex);
    statsPress.iSize = stoi(strNumber);

    if (!statsPress.iSize)
    {
        showError(L"User::parseEntry()::PRESS_SIZE");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get expectation of delays between key presses.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strNumber = strTemp.substr(0, iDelimiterIndex);
    statsPress.dExpectation = stod(strNumber);

    if (!statsPress.dExpectation)
    {
        showError(L"User::parseEntry()::PRESS_EXPECTATION");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get variance of delays between key presses.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strNumber = strTemp.substr(0, iDelimiterIndex);
    statsPress.dVariance = stod(strNumber);

    if (!statsPress.dVariance)
    {
        showError(L"User::parseEntry()::PRESS_VARIANCE");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get size of key hold data.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strNumber = strTemp.substr(0, iDelimiterIndex);
    statsHold.iSize = stoi(strNumber);

    if (!statsHold.iSize)
    {
        showError(L"User::parseEntry()::HOLD_SIZE");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get expectation of key hold durations.
    strTemp = strTemp.substr(iBeginIndex);
    iDelimiterIndex = strTemp.find(CREDS_DELIMITER);
    strNumber = strTemp.substr(0, iDelimiterIndex);
    statsHold.dExpectation = stod(strNumber);

    if (!statsHold.dExpectation)
    {
        showError(L"User::parseEntry()::HOLD_EXPECTATION");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;

    // Get variance of key hold durations.
    iDelimiterIndex = strTemp.find(L"\r\n");
    strNumber = strTemp.substr(iBeginIndex, iDelimiterIndex);
    statsHold.dVariance = stod(strNumber);

    if (!statsHold.dVariance)
    {
        showError(L"User::parseEntry()::HOLD_VARIANCE");
        return -1;
    }

    // Store parsed data.
    setUsername(strUsername);
    setPassword(strPassword);
    setPressStats(statsPress);
    setHoldStats(statsHold);

    return 0;
}

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
int addMainMenu(HWND hwndMain)
{
    // Create and add menu bar.
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuFile = CreateMenu();
    HMENU hMenuHelp = CreateMenu();

    if (!hMenuBar || !hMenuFile || !hMenuHelp)
    {
        showError(L"addMainMenu::CreateMenu");
        return -1;
    }

    // Create menu options.
    if (!AppendMenu(hMenuHelp, MF_STRING, MAIN_ABOUT, L"About program"))
    {
        showError(L"addMainMenu::AppendMenu::ABOUT");
        return -1;
    }

    if (!AppendMenu(hMenuFile, MF_STRING, MAIN_LOGIN, L"Login"))
    {
        showError(L"addMainMenu::AppendMenu::LOGIN");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_STRING, MAIN_REGISTER, L"Register"))
    {
        showError(L"addMainMenu::AppendMenu::REGISTER");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL))
    {
        showError(L"addMainMenu::AppendMenu::SEPARATOR");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_STRING, WM_DESTROY, L"Exit"))
    {
        showError(L"addMainMenu::AppendMenu::EXIT");
        return -1;
    }

    // Add created options to menu bar.
    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuFile, L"File"))
    {
        showError(L"addMainMenu::FILE::AppendMenu");
        return -1;
    }
    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuHelp, L"Help"))
    {
        showError(L"addMainMenu::HELP::AppendMenu");
        return -1;
    }

    if (!SetMenu(hwndMain, hMenuBar))
    {
        showError(L"addMainMenu::SetMenu");
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

    HWND hwndUsername = CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_USERNAME, NULL, NULL
    );
        
    if (!hwndUsername)
    {
        showError(L"addFormControls::EDIT::USERNAME");
        return -1;
    }

    // Limit size of input to username EDIT control.
    SendMessage(hwndUsername, EM_LIMITTEXT, USERNAME_MAXSIZE, 0);

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

    SendMessage(hwndPassword, EM_LIMITTEXT, PASSWORD_MAXSIZE, 0);

    // Subclass the password EDIT control to gather keystroke data.
    SetWindowSubclass(hwndPassword, &KeystrokeProc, 0, 0);

    if (!CreateWindow(
        L"BUTTON", L"Submit",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 10,
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
        ELEMENT_OFFSET * 10,
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
int addFormMenu(HWND hwndForm)
{
    // Create and add menu bar.
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuHelp = CreateMenu();

    if (!hMenuBar || !hMenuHelp)
    {
        showError(L"addFormMenu::CreateMenu");
        return -1;
    }

    // Create menu options.
    if (!AppendMenu(hMenuHelp, MF_STRING, FORM_ABOUT, L"About form"))
    {
        showError(L"addFormMenu::AppendMenu::ABOUT");
        return -1;
    }

    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuHelp, L"Help"))
    {
        showError(L"addFormMenu::HELP::AppendMenu");
        return -1;
    }

    if (!SetMenu(hwndForm, hMenuBar))
    {
        showError(L"addFormMenu::SetMenu");
        return -1;
    }

    return 0;
}
// Optimization of sendData must be turned off.
// Otherwise it isn't executed.
#pragma optimize( "", off )
void sendData(HWND hwndReceiver, HWND hwndSender, const std::vector<DWORD>& vData, DWORD dwID)
{
    // Create struct to contain data.
    COPYDATASTRUCT cds{};

    cds.dwData = dwID;
    cds.cbData = vData.size() * sizeof(DWORD);
    cds.lpData = (VOID*)vData.data();

    // Send data to target.
    SendMessage(
        hwndReceiver,
        WM_COPYDATA,
        (WPARAM)hwndSender,
        (LPARAM)(LPVOID)&cds
    );
}
#pragma optimize( "", on )
void clearEditControl(HWND hwndParent, int iWindowID)
{
    // Remove entered password.
    HWND hwndEdit = GetDlgItem(hwndParent, iWindowID);

    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)L"");

    // Set keyboard focus to the password EDIT control.
    SetFocus(hwndEdit);
}

int loginUser(User* pUser, std::wstring& strCreds, HWND hwndForm)
{
    // Read and check the username.
    std::wstring strUsername;
    char iResult = readInput(strUsername, hwndForm, FORM_USERNAME);

    if (iResult != 0)
    {
        if (iResult == READ_EMPTY)
        {
            MessageBox(
                NULL,
                L"Username is empty",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }

        // Cleanup.
        clearEditControl(hwndForm, FORM_USERNAME);

        return -1;
    }

    pUser->setUsername(strUsername);

    // Read and check the password.
    std::wstring strPassword;
    iResult = readInput(strPassword, hwndForm, FORM_PASSWORD);

    if (iResult != 0)
    {
        return 0;
    }

    pUser->setPassword(strPassword);

    // Get user credentials from credentials file.
    User userCreds;

    if (userCreds.getCreds(pUser->getUsername(), strCreds) < 0)
    {
        return 0;
    }

    // Compare passwords.
    if (userCreds.getPassword().compare(pUser->getPassword()) != 0)
    {
        return 0;
    }

    // Compare statistical data.
    Stats statsPressCreds = userCreds.getPressStats();
    Stats statsPressAuth = pUser->getPressStats();
    Stats statsHoldCreds = userCreds.getHoldStats();
    Stats statsHoldAuth = pUser->getHoldStats();

    if (compareStats(statsPressCreds, statsPressAuth) == 1
        && compareStats(statsHoldCreds, statsHoldAuth) == 1)
    {
        // Update user's entry in the credentials string.
        if (pUser->setCreds(strCreds) < 0)
        {
            return -1;
        }

        // Overwrite the file with credentials.
        if (writeCreds(strCreds, std::ios::out) < 0)
        {
            return -1;
        }

        return 1;
    }

    return 0;
}
int registerUser(User* pUser, std::wstring& strCreds, HWND hwndForm)
{
    // Read and check the username.
    std::wstring strUsername;
    char iResult = readInput(strUsername, hwndForm, FORM_USERNAME);

    if (iResult != 0)
    {
        if (iResult == READ_EMPTY)
        {
            MessageBox(
                NULL,
                L"Username field is empty",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }
        else if (iResult == READ_FORBIDDEN)
        {
            MessageBox(
                NULL,
                L"Username contains forbidden characters",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }

        // Cleanup.
        clearEditControl(hwndForm, FORM_PASSWORD);
        clearEditControl(hwndForm, FORM_USERNAME);

        return -1;
    }

    pUser->setUsername(strUsername);

    // Check if the user is already registered.
    iResult = pUser->registered(strCreds);

    if (iResult == 1)
    {
        MessageBox(
            NULL,
            L"User is already registered",
            L"Alert",
            MB_OK | MB_ICONWARNING
        );

        clearEditControl(hwndForm, FORM_PASSWORD);
        clearEditControl(hwndForm, FORM_USERNAME);

        return -1;
    }
    else if (iResult < 0)
    {
        return -1;
    }

    // Read and check the password.
    std::wstring strPassword;
    iResult = readInput(strPassword, hwndForm, FORM_PASSWORD);

    if (iResult != 0)
    {
        if (iResult == READ_EMPTY)
        {
            MessageBox(
                NULL,
                L"Password field is empty",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }
        else if (iResult == READ_TOOSHORT)
        {
            MessageBox(
                NULL,
                L"Password is too short",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }
        else if (iResult == READ_FORBIDDEN)
        {
            MessageBox(
                NULL,
                L"Password contains forbidden characters",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }

        // Cleanup.
        clearEditControl(hwndForm, FORM_PASSWORD);
        clearEditControl(hwndForm, FORM_USERNAME);

        return -1;
    }

    pUser->setPassword(strPassword);

    // Update the credentials string.
    if (pUser->setCreds(strCreds) < 0)
    {
        return -1;
    }

    // Overwrite the file with credentials.
    if (writeCreds(strCreds, std::ios::out) < 0)
    {
        return -1;
    }

    return 0;
}
int readInput(std::wstring& strInput, HWND hwndForm, int iEditID)
{
    // Get handle to the EDIT control.
    HWND hwndEdit = GetDlgItem(hwndForm, iEditID);

    if (!hwndEdit)
    {
        showError(L"readInput::GetDlgItem");
        return -1;
    }

    // Define maximum and minimum size of input.
    int iInputMaxSize, iInputMinSize = 0;

    if (iEditID == FORM_USERNAME)
    {
        iInputMaxSize = USERNAME_MAXSIZE;
    }
    else if (iEditID == FORM_PASSWORD)
    {
        iInputMaxSize = PASSWORD_MAXSIZE;
        iInputMinSize = PASSWORD_MINSIZE;
    }

    // Get username.
    strInput.resize(iInputMaxSize + 1, 0);
    {
        int iSize = GetWindowText(hwndEdit, &strInput[0], iInputMaxSize);

        // Check if reading didn't fail.
        if (GetLastError())
        {
            showError(L"readInput::GetWindowText");
            return READ_ERROR;
        }
        // Check if the input control is empty.
        if (!iSize)
        {
            return READ_EMPTY;
        }
        // Check if input is not too short.
        if (iInputMinSize && iSize < iInputMinSize)
        {
            return READ_TOOSHORT;
        }

        // Resize buffer to remove extra null bytes.
        strInput.resize(iSize);

        // Check if the username contains banned characters.
        if (!valid(strInput))
        {
            return READ_FORBIDDEN;
        }
    }

    return 0;
}
int valid(const std::wstring& strInput)
{
    // Check banned characters.
    for (int i = 0; i < wcslen(cstrBanned); i++)
    {
        if (strInput.find(cstrBanned[i]) != std::wstring::npos)
        {
            return 0;
        }
    }

    return 1;
}
int readCreds(std::wstring& strCreds)
{
    // Open the file with credentials for reading.
    std::ifstream credsFile(cstrCredsPath, std::ios::binary);

    if (!credsFile.is_open())
    {
        // If the file doesn't exist, create it.
        if (writeCreds(L"", std::ios::out) < 0)
        {
            return -1;
        }

        // Try to open the created file.
        credsFile.open(cstrCredsPath, std::ios::binary);

        if (!credsFile.is_open())
        {
            showError(L"readCreds::std::ifstream::is_open");
            return -1;
        }
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

    // Remove extra null bytes.
    strCreds.erase(
        std::find(strCreds.begin(), strCreds.end(), '\0'),
        strCreds.end()
    );

    return 0;
}
int writeCreds(const std::wstring& strCreds, std::ios::openmode mode)
{
    // Open file for writing.
    std::ofstream credsFile(cstrCredsPath, mode | std::ios::binary);

    if (!credsFile.is_open())
    {
        showError(L"writeCreds::std::ofstream::open");
        return -1;
    }

    // Convert credentials to UTF-8.
    std::string strMultibyteCreds;

    if (toMultibyte(strMultibyteCreds, strCreds) < 0)
    {
        showError(L"writeCreds::toMultibyte");
        return -1;
    }

    // Remove extra null bytes.
    strMultibyteCreds.erase(
        std::find(strMultibyteCreds.begin(), strMultibyteCreds.end(), '\0'),
        strMultibyteCreds.end()
    );

    // Write converted credentials to file.
    credsFile << strMultibyteCreds;

    // Cleanup.
    credsFile.close();

    return 0;
}
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

double expectation(const std::vector<DWORD>& vData)
{
    double dExp = 0;

    for (DWORD i = 0; i < vData.size(); i++)
    {
        dExp += vData[i];
    }

    dExp /= vData.size();

    return dExp;
}
double variance(const std::vector<DWORD>& vData)
{
    double dVar = 0;

    double dExp = expectation(vData);

    return variance(dExp, vData);
}
double variance(double dExp, const std::vector<DWORD>& vData)
{
    double dVar = 0;

    for (DWORD i = 0; i < vData.size(); i++)
    {
        dVar += (vData[i] - dExp) * (vData[i] - dExp);
    }

    dVar /= vData.size() - 1;

    return std::sqrt(dVar);
}
void excludeErrors(std::vector<DWORD>* vData)
{
    // Create a vector copy for storing not excluded elements.
    std::vector<DWORD> vExcluded;

    for (int i = 0; i < vData->size(); i++)
    {
        // Create a vector without the i-th element.
        std::vector<DWORD> vTemp = *vData;

        vTemp.erase(vTemp.begin() + i);

        // Calculate Student's coefficient.
        double M = expectation(vTemp);
        double S = variance(M, vTemp);
        double t = (vData->at(i) - M) / S;

        // Get absolute value.
        if (t < 0) t *= -1;

        // Check if the i-th element should be excluded.
        if (t <= t_05[vData->size() - 1])
        {
            vExcluded.push_back(vData->at(i));
        }
    }

    // Remove marked elements.
    *vData = vExcluded;
}
Stats calculateStats(const std::vector<DWORD>& vData)
{
    // Calculate statistics.
    double dExpectation = expectation(vData);
    double dVariance = variance(dExpectation, vData);

    return { (int)vData.size(), dExpectation, dVariance };
}
int compareStats(const Stats& statsIn1, const Stats& statsIn2)
{
    int n = statsIn1.iSize;

    // Calculate variances.
    double Smax = max(statsIn1.dVariance, statsIn2.dVariance);
    double Smin = min(statsIn1.dVariance, statsIn2.dVariance);

    // Calculate F-coefficient.
    double F = Smax / Smin;

    // Check calculated F-coefficient.
    if (F > f_05[n - 1])
    {
        return 0;
    }

    // Calculate Student's coefficient.
    double S = std::sqrt((statsIn1.dVariance * statsIn1.dVariance
        + statsIn2.dVariance * statsIn2.dVariance)
        * (n - 1) / (2 * n - 1));

    double t = (statsIn1.dExpectation - statsIn2.dExpectation) * std::sqrt(n / 2) / S;

    if (t < 0) t *= -1;

    // Check calculated Student's coefficient.
    if (t > t_05[n - 1])
    {
        return 0;
    }

    return 1;
}

void showError(const std::wstring& strError)
{
    std::wstringstream sstr;

    sstr << strError << L". Error code: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring str = sstr.str();

    MessageBox(NULL, str.c_str(), L"Error", MB_OK | MB_ICONERROR);
}
