#include "biometrics.h"

const wchar_t wcBanned[] = { L'%', CREDS_DELIMITER[0] };
const wchar_t cstrCredsPath[] = L"./creds.txt";

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Main class creation and registration.
    WNDCLASSEX wc = { };

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_QUESTION);

    if (!RegisterClassEx(&wc))
    {
        showError(L"wWinMain::RegisterClassEx::MAIN");
        return -1;
    }

    // Form class creation and registration.
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = FormProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = FORM_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClassEx(&wc))
    {
        showError(L"wWinMain::RegisterClassEx::FORM");
        return -1;
    }

    // Create the window.
    HWND hwnd;

    hwnd = CreateWindow(
        MAIN_CLASS, L"Biometrics App",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        MAIN_WIDTH, MAIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL)
    {
        showError(L"wWinMain::CreateWindow");
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    MSG msg = { };

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (addMainControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case MAIN_LOGIN:
        {
            // Open login form.
            if (openForm(hwnd, FORM_LOGIN) < 0)
            {
                break;
            }

            // Hide identification window.
            ShowWindow(hwnd, SW_HIDE);

            break;
        }
        case MAIN_REGISTER:
        {
            // Open registration form.
            if (openForm(hwnd, FORM_REGISTER) < 0)
            {
                break;
            }

            // Disable main window.
            //EnableWindow(hwnd, FALSE);

            // Hide identification window.
            ShowWindow(hwnd, SW_HIDE);

            break;
        }
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK FormProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get main window handle.
    static HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

    switch (uMsg)
    {
    case WM_CREATE:
        if (addFormControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_CLOSE:
    {
        // Show main window and close the form.
        ShowWindow(hwndOwner, SW_SHOW);
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case FORM_SUBMIT:
            break;
        case FORM_CANCEL:
            // Show main window and close the form.
            ShowWindow(hwndOwner, SW_SHOW);
            DestroyWindow(hwnd);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
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

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 7,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndForm, (HMENU)FORM_PASSWORD, NULL, NULL
    ))
    {
        showError(L"addFormControls::EDIT::PASSWORD");
        return -1;
    }

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

int identify(std::wstring& strUsername, HWND hwndForm)
{
    // Create buffer for username.
    strUsername.resize(USERNAME_SIZE + 1, 0);

    // Get handle to username field.
    HWND hwndUsername = GetDlgItem(hwndForm, FORM_USERNAME);

    if (hwndUsername == NULL)
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

    // Check if user is registered.
    {
        // Read all credentials.
        std::string strMultibyteCreds;

        if (readCreds(strMultibyteCreds) < 0)
        {
            return -1;
        }

        // Convert data.
        std::wstring strWideCreds;

        if (toWide(strWideCreds, strMultibyteCreds) < 0)
        {
            return -1;
        }

        // Try to find user's entry.
        if (findEntry(strUsername, strWideCreds) == std::vector<int>{ -1, -1 })
        {
            return 0;
        }
    }

    return 1;
}

int openForm(HWND hwndOwner, const std::wstring& strFormTitle)
{
    // Create user window.
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    if (!hInstance)
    {
        showError(L"openForm::GetWindowLongW");
        return -1;
    }

    // Create and show user window.
    HWND hwndUser = CreateWindow(
        FORM_CLASS, strFormTitle.c_str(),
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        FORM_WIDTH, FORM_HEIGHT,
        hwndOwner, NULL, hInstance, NULL
    );

    if (hwndUser)
    {
        ShowWindow(hwndUser, SW_SHOW);
        UpdateWindow(hwndUser);
    }
    else
    {
        showError(L"openForm::CreateWindow");
        return -1;
    }

    return 0;
}

int readCreds(std::string& strCreds)
{
    // Open file for reading.
    std::ifstream credsFile(cstrCredsPath, std::ios::binary);

    if (!credsFile.is_open())
    {
        showError(L"readCreds::std::ifstream::is_open");
        return -1;
    }

    // Read file contents
    strCreds.assign(
        (std::istreambuf_iterator<char>(credsFile)),
        (std::istreambuf_iterator<char>())
    );

    // Cleanup.
    credsFile.close();

    return 0;
}

std::vector<int> findEntry(const std::wstring& strUsername, const std::wstring& strCreds)
{
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

void showError(const std::wstring& cstrError)
{
    std::wstringstream sstr;

    sstr << cstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring str = sstr.str();

    MessageBox(NULL, str.c_str(), L"Error", MB_OK);
}

void showValue(const std::wstring& cstrName, auto aValue)
{
    std::wstringstream sstrValue;

    sstrValue << aValue << std::endl;

    std::wstring strValue = sstrValue.str();

    MessageBox(NULL, strValue.c_str(), cstrName.c_str(), 0);
}
