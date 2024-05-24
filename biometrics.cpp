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
    static std::vector<DWORD> vPressData, vHoldData;

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
            vPressData.clear();
            vHoldData.clear();
            iType = 0;

            // Remove password EDIT control subclass.
            HWND hwndPassword = GetDlgItem(hwnd, FORM_PASSWORD);

            RemoveWindowSubclass(hwndPassword, KeystrokeProc, 0);
        }

        // Enable and show the main window.
        {
            // Get main window handle.
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

            EnableWindow(hwndOwner, TRUE);
            ShowWindow(hwndOwner, SW_SHOW);
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
            vPressData.assign(
                (DWORD*)pcds->lpData,
                (DWORD*)pcds->lpData + pcds->cbData / sizeof(DWORD)
            );

            //DBG_dwShowArr(vPressData.data(), vPressData.size());

            break;
        }

        case SRID_HOLD:
        {
            vHoldData.assign(
                (DWORD*)pcds->lpData,
                (DWORD*)pcds->lpData + pcds->cbData / sizeof(DWORD)
            );

            //DBG_dwShowArr(vHoldData.data(), vHoldData.size());

            break;
        }
        }

        // If all data was received, proceed to next steps.
        if (!vPressData.empty() && !vHoldData.empty())
        {
            // Exclude errors from gathered data.
            excludeErrors(&vPressData);
            excludeErrors(&vHoldData);

            // Authenticate or register the user according to the form type.
            if (iType == FORM_LOGIN)
                SendMessage(hwnd, WM_LOGIN, 0, 0);
            else if (iType == FORM_REGISTER)
                SendMessage(hwnd, WM_REGISTER, 0, 0);
        }
        else if (vPressData.empty() && vHoldData.empty())
        {
            MessageBox(
                NULL,
                L"Failed to gather keystroke biometrics data",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }

        return 0;
    }

    // Initiate user authentication.
    case WM_LOGIN:
    {
        // Read credentials from the file.
        std::wstring strCreds;

        if (readCreds(strCreds) < 0)
        {
            return -1;
        }

        // Calculate user's statistical data.
        User user;

        user.setPressStats(vPressData);
        user.setHoldStats(vHoldData);

        // Check if user is registered.
        if (identifyUser(&user, strCreds, hwnd) == 1)
        {
            // Check entered credentials.
            if (authenticateUser(&user, strCreds, hwnd) == 1)
            {
                // Get the handle to the main window.
                HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

                if (!hwndOwner)
                {
                    showError(L"FormProc::GetWindow");
                    return -1;
                }

                // Hide the main window.
                ShowWindow(hwndOwner, SW_HIDE);
                // Hide the form.
                ShowWindow(hwnd, SW_HIDE);

                // Authorize user.
                MessageBox(
                    NULL,
                    L"You have successfully logged in.\n"
                    L"Press \"OK\" or close this message to return to the main window.",
                    L"Notification",
                    MB_OK
                );

                // Close the form.
                SendMessage(hwnd, WM_CLOSE, 0, 0);

                return 0;
            }
        }

        MessageBox(
            NULL,
            L"Incorrect username/password/biometrics.",
            L"Alert",
            MB_OK
        );

        // Cleanup.
        vPressData.clear();
        vHoldData.clear();

        return -1;
    }

    // Register a new user.
    case WM_REGISTER:
    {
        // Read credentials from the file.
        std::wstring strCreds;

        if (readCreds(strCreds) < 0)
        {
            return -1;
        }

        // Calculate user's statistical data.
        User user;

        user.setPressStats(vPressData);
        user.setHoldStats(vHoldData);

        if (registerUser(&user, strCreds, hwnd) == 0)
        {
            // Get the handle to the main window.
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

            if (!hwndOwner)
            {
                showError(L"FormProc::GetWindow");
                return -1;
            }

            // Hide the main window.
            ShowWindow(hwndOwner, SW_HIDE);
            // Hide the form.
            ShowWindow(hwnd, SW_HIDE);

            // Authorize user.
            MessageBox(
                NULL,
                L"You have successfully registered.\n"
                L"Press \"OK\" or close this message to return to the main window.",
                L"Notification",
                MB_OK
            );

            // Close the form.
            SendMessage(hwnd, WM_CLOSE, 0, 0);

            return 0;
        }

        // Cleanup.
        vPressData.clear();
        vHoldData.clear();

        return -1;
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
    // Vector that stores held keys.
    static std::vector<KeyPress> vKeyHeld;

    // Vectors for storing keystroke data:
    // - duration of delays between key presses.
    static std::vector<DWORD> vPressData;
    // - duration of a key hold.
    static std::vector<DWORD> vHoldData;

    // Variable that stores time of the previous key press.
    static DWORD dwPrevPressTime = 0;

    switch (uMsg)
    {
    // Send gathered data to the form.
    case WM_SENDKEYSTROKEDATA:
    {
        // Get the handle to the form.
        HWND hwndParent = GetParent(hwnd);

        if (!hwndParent)
        {
            showError(L"KeystrokeProc::GetParent");
            return -1;
        }

        // Send keystroke data.
        sendData(hwndParent, hwnd, vPressData, SRID_PRESS);
        sendData(hwndParent, hwnd, vHoldData, SRID_HOLD);
        
        // Cleanup.
        vKeyHeld.clear();
        vPressData.clear();
        vHoldData.clear();
        dwPrevPressTime = 0;

        break;
    }

    case WM_KEYDOWN:
    {
        // Submit on ENTER press.
        if (wParam == VK_RETURN)
        {
            HWND hwndParent = GetParent(hwnd);

            if (!hwndParent)
            {
                showError(L"KeystrokeProc::GetParent");
                return -1;
            }

            SendMessage(hwndParent, WM_COMMAND, FORM_SUBMIT, 0);
            break;
        }
        // Clear everything on BACKSPACE and DELETE press
        else if (wParam == VK_BACK || wParam == VK_DELETE)
        {
            // Remove entered text.
            SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)L"");

            // Cleanup.
            vKeyHeld.clear();
            vPressData.clear();
            vHoldData.clear();
            dwPrevPressTime = 0;
        }
        // Calculate time between key presses.
        // Ignore any key except for numbers and letters.
        else if (wParam >= 0x30 && wParam <= 0x5A)
        {
            if (!dwPrevPressTime)
            {
                dwPrevPressTime = timeGetTime();
            }

            // Measure and store the time of a key press.
            DWORD dwCurrPressTime = timeGetTime();

            vPressData.push_back(dwCurrPressTime - dwPrevPressTime);

            // Enqueue key press information.
            KeyPress kp = { wParam, dwCurrPressTime };

            vKeyHeld.push_back(kp);

            // Reset the previous key press time.
            dwPrevPressTime = dwCurrPressTime;
        }
        break;
    }

    case WM_KEYUP:
    {
        // Calculate duration of key holds.
        if (wParam >= 0x30 && wParam <= 0x5A)
        {
            // Find a released key.
            int i = 0;

            while (i < vKeyHeld.size())
            {
                if (vKeyHeld[i].wKeyCode == wParam)
                {
                    // Calculate key hold duration.
                    vHoldData.push_back(timeGetTime() - vKeyHeld[i].dwTime);

                    // Remove the released key from the queue.
                    vKeyHeld.erase(vKeyHeld.begin() + i);

                    break;
                }

                i++;
            }
        }
        break;
    }

    default:
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }
}
