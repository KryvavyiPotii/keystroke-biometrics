#include "biometrics.h"

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
    {
        if (addMainControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

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
            // Create a form.
            HWND hwndForm = createForm(hwnd, FORM_LOGIN);

            if (!hwndForm)
            {
                break;
            }

            // Set form type.
            SendMessage(hwndForm, WM_FORMTYPE, FORM_LOGIN, 0);

            // Disable the main window.
            EnableWindow(hwnd, FALSE);

            break;
        }
        case MAIN_REGISTER:
        {
            HWND hwndForm = createForm(hwnd, FORM_REGISTER);

            if (!hwndForm)
            {
                break;
            }

            SendMessage(hwndForm, WM_FORMTYPE, FORM_REGISTER, 0);

            EnableWindow(hwnd, FALSE);

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
    // Set form type (login or register) for data processing.
    static int iType = 0;

    // Create buffers for keystroke data.
    static std::vector<DWORD> vPress, vHold;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        if (addFormControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    // Get the form type from the main window.
    case WM_FORMTYPE:
    {
        iType = wParam;
        return 0;
    }

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
        // Cleanup.
        {
            // Remove edit control subclass.
            HWND hwndPassword = GetDlgItem(hwnd, FORM_PASSWORD);

            RemoveWindowSubclass(hwndPassword, KeystrokeProc, 0);
        }

        // Enable and show the main window.
        {
            // Get main window handle.
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

            EnableWindow(hwndOwner, TRUE);
            ShowWindow(hwndOwner, SW_SHOWNORMAL);
        }

        DestroyWindow(hwnd);
        return 0;
    }

    // Receive data from the password EDIT control.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
        
        switch (pcds->dwData)
        {
        case SRID_PRESS:
        {
            // Store received data.
            vPress.assign(
                (DWORD*)pcds->lpData,
                (DWORD*)pcds->lpData + pcds->cbData / sizeof(DWORD)
            );

            DBG_dwShowArr(vPress.data(), vPress.size());

            break;
        }

        case SRID_HOLD:
        {
            vHold.assign(
                (DWORD*)pcds->lpData,
                (DWORD*)pcds->lpData + pcds->cbData / sizeof(DWORD)
            );

            break;
        }
        }

        // If all data was received, proceed to next steps.
        if (!vPress.empty() && !vHold.empty())
        {
            // Create vectors with excluded errors.
            excludeErrors(vPress);
            excludeErrors(vHold);

            // Authenticate or register the user according to the form's type.
            if (iType == FORM_LOGIN)
                SendMessage(hwnd, WM_LOGIN, 0, 0);
            if (iType == FORM_REGISTER)
                SendMessage(hwnd, WM_REGISTER, 0, 0);
        }

        return 0;
    }

    // Initiate user authentication.
    case WM_LOGIN:
    {
        // Read all credentials from the file.
        std::wstring strCreds;

        if (readCreds(strCreds) < 0)
        {
            return -1;
        }

        User user;

        // Check entered credentials.
        if (identify(&user, strCreds, hwnd) == 1)
        {
            if (authenticate(&user, vPress, vHold, strCreds, hwnd) == 1)
            {
                MessageBox(
                    NULL,
                    L"You have successfully logged in.\n"
                    L"Press \"OK\" or close this message to return to the main window.",
                    L"Notification",
                    MB_OK
                );
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
        }

        MessageBox(
            NULL,
            L"Incorrect username/password.",
            L"Alert",
            MB_OK
        );

        return -1;
    }

    // Register a new user.
    case WM_REGISTER:
    {
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case FORM_SUBMIT:
        {
            // Signal the password EDIT control to send gathered data.
            HWND hwndPassword = GetDlgItem(hwnd, FORM_PASSWORD);

            SendMessage(hwndPassword, WM_SENDKEYSTROKEDATA, 0, 0);

            break;
        }

        case FORM_CANCEL:
        {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        }
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK KeystrokeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    // Vectors for storing keystroke data:
    // - duration of delays between key presses.
    static std::vector<DWORD> vPress;
    // - duration of a key hold.
    static std::vector<DWORD> vHold;

    // Variables for duration calculation.
    static DWORD currPressTime, prevPressTime = 0;
    static DWORD holdTime, releaseTime;

    switch (uMsg)
    {
    // Send gathered data to the form.
    case WM_SENDKEYSTROKEDATA:
    {
        // Get the handle to the form.
        HWND hwndParent = GetParent(hwnd);

        if (!hwndParent)
        {
            showError(L"KeystrokeProc::GetWindow");
            break;
        }

        // Send keystroke data.
        sendData(hwndParent, hwnd, vPress, SRID_PRESS);
        sendData(hwndParent, hwnd, vHold, SRID_HOLD);
        
        // Cleanup.
        prevPressTime = 0;
        vPress.clear();
        vHold.clear();

        break;
    }

    case WM_KEYDOWN:
    {
        // Calculate time between key presses.
        // Ignore special keys.
        if (wParam != VK_SHIFT)
        {
            // Set the key hold time.
            holdTime = timeGetTime();

            // Check if the pressed key is the first one.
            if (prevPressTime)
            {
                // Calculate time from the previous key press.
                currPressTime = timeGetTime();
                vPress.push_back(currPressTime - prevPressTime);
                prevPressTime = currPressTime;
            }
        }
        break;
    }

    case WM_KEYUP:
    {
        // Calculate time between key presses.
        if (wParam != VK_SHIFT)
        {
            // Set the previous key press time.
            prevPressTime = timeGetTime();

            // Calculate duration of the key hold.
            releaseTime = timeGetTime();
            vHold.push_back(releaseTime - holdTime);
        }
        break;
    }

    default:
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }
}
