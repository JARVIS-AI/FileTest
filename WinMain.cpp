/*****************************************************************************/
/* WinMain.cpp                            Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* A file that simulates access on a file                                    */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 14.07.03  1.00  Lad  The first version of WinMain.cpp                     */
/*****************************************************************************/

#include "FileTest.h"
#include "resource.h"

#pragma comment(lib, "Comctl32.lib")

//-----------------------------------------------------------------------------
// Global variables

TContextMenu g_ContextMenus[MAX_CONTEXT_MENUS];
TToolTip g_Tooltip;
DWORD g_dwWinVer;
DWORD g_dwWinBuild;
TCHAR g_szInitialDirectory[MAX_PATH];
DWORD g_dwMenuCount = 0;

//-----------------------------------------------------------------------------
// Local functions

inline bool IsCommandSwitch(LPCTSTR szArg)
{
    return (szArg[0] == _T('/') || szArg[0] == _T('-'));
}

static void SetTokenObjectIntegrityLevel(DWORD dwIntegrityLevel)
{
    SID_IDENTIFIER_AUTHORITY Sia = SECURITY_MANDATORY_LABEL_AUTHORITY;
    SECURITY_DESCRIPTOR sd;
    HANDLE hToken;
    DWORD dwLength;
    PACL pAcl;
    PSID pSid;

    // Do nothing on OSes where mandatory ACEs are not supported
    if(pfnAddMandatoryAce == NULL)
        return;

    // Initialize blank security descriptor
    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        return;

    // Allocate mandatory label SID
    if(!AllocateAndInitializeSid(&Sia, 1, dwIntegrityLevel, 0, 0, 0, 0, 0, 0, 0, &pSid))
        return;

    // Open current token
    if(!OpenThreadToken(GetCurrentThread(), WRITE_OWNER, TRUE, &hToken))
    {
        if(GetLastError() == ERROR_NO_TOKEN)
            OpenProcessToken(GetCurrentProcess(), WRITE_OWNER, &hToken);
    }

    // If succeeded, set the integrity level
    if(hToken != NULL)
    {
        // Create ACL
        dwLength = sizeof(ACL) + sizeof(SYSTEM_MANDATORY_LABEL_ACE) - sizeof(DWORD) + GetLengthSid(pSid);
        pAcl = (PACL)HeapAlloc(g_hHeap, 0, dwLength);
        if(pAcl != NULL)
        {
            if(InitializeAcl(pAcl, dwLength, ACL_REVISION))
            {
                if(pfnAddMandatoryAce(pAcl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, pSid))
                {
                    NtSetSecurityObject(hToken, LABEL_SECURITY_INFORMATION, &sd);
                }
            }

            HeapFree(g_hHeap, 0, pAcl);
        }
    }

    FreeSid(pSid);
}

BOOL CALLBACK EnumMenusProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR /* lParam */)
{
    // Only take menus
    if(lpszType == RT_MENU)
    {
        // Debug code check
        assert(g_dwMenuCount < MAX_CONTEXT_MENUS);

        // Check if the number of context menus is in range
        if(g_dwMenuCount < MAX_CONTEXT_MENUS)
        {
            // Insert the menu entry
            g_ContextMenus[g_dwMenuCount].szMenuName = lpszName;
            g_ContextMenus[g_dwMenuCount].hMenu = LoadMenu(hModule, lpszName);

            // Increment the menu count
            g_dwMenuCount++;
        }
    }

    // Keep enumerating
    return TRUE;
}

#ifdef _DEBUG
static void DebugCode_TEST()
{}
#endif

//-----------------------------------------------------------------------------
// WinMain

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
    TFileTestData * pData;
    DWORD dwDesiredAccess = GENERIC_READ;
    DWORD dwShareAccess = FILE_SHARE_READ;
    DWORD dwCreateOptions = 0;
    DWORD dwCopyFileFlags = 0;
    DWORD dwMoveFileFlags = 0;
    bool bAsynchronousOpen = false;
    int nFileNameIndex = 0;

    // Initialize the instance
    InitInstance(hInstance);
    InitCommonControls();

    // Get the Windows version
    g_dwWinVer = GetWindowsVersion();

    // Allocate and fill our working structure with command line parameters
    pData = (TFileTestData *)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, sizeof(TFileTestData));

    // Parse command line arguments
    for(int i = 1; i < __argc; i++)
    {
        // If the argument a file name?
        if(!IsCommandSwitch(__targv[i]))
        {
            switch(nFileNameIndex)
            {
                case 0: // The first file name argument
                    StringCchCopy(pData->szFileName1, _countof(pData->szFileName1), __targv[i]);
                    nFileNameIndex++;
                    break;

                case 1: // The second file name argument
                    StringCchCopy(pData->szFileName2, _countof(pData->szFileName2), __targv[i]);
                    nFileNameIndex++;
                    break;

                case 2: // The directory file name argument
                    StringCchCopy(pData->szDirName, _countof(pData->szFileName2), __targv[i]);
                    nFileNameIndex++;
                    break;
            }
        }
        else
        {
            LPCTSTR szArg = __targv[i] + 1;

            // Check for default read+write access
            if(!_tcsnicmp(szArg, _T("DesiredAccess:"), 14))
                Text2Hex32(szArg+14, &dwDesiredAccess);

            // Check for default share read+write
            if(!_tcsnicmp(szArg, _T("ShareAccess:"), 12))
                Text2Hex32(szArg+12, &dwShareAccess);

            // Check for changed create options
            if(!_tcsnicmp(szArg, _T("CreateOptions:"), 14))
                Text2Hex32(szArg+14, &dwCreateOptions);

            if(!_tcsnicmp(szArg, _T("CopyFileFlags:"), 14))
                Text2Hex32(szArg+14, &dwCopyFileFlags);

            if(!_tcsnicmp(szArg, _T("MoveFileFlags:"), 14))
                Text2Hex32(szArg+14, &dwMoveFileFlags);

            // Check for asynchronous open
            if(!_tcsicmp(szArg, _T("AsyncOpen")))
                bAsynchronousOpen = true;
        }
    }

    // Set default file name
    if(pData->szFileName1[0] == 0)
    {
        StringCchCopy(pData->szFileName1, _countof(pData->szFileName1), _T("C:\\TestFile.bin"));
        pData->IsDefaultFileName1 = TRUE;
    }

    //
    // DEVELOPMENT CODE: Build the NT status table from the NTSTATUS.h
    //

//  BuildNtStatusTableFromNTSTATUS_H();
//  VerifyNtStatusTable();

    //
    // Resolve the dynamic loaded APIs
    //

    ResolveDynamicLoadedAPIs();

    //
    // On Vista or newer, set the required integrity level of our token object
    // to lowest possible value. This will allow us to open our token even if the user
    // lowers the integrity level.
    //

    SetTokenObjectIntegrityLevel(SECURITY_MANDATORY_UNTRUSTED_RID);

    //
    // Save the application initial directory
    //

    GetCurrentDirectory(_countof(g_szInitialDirectory), g_szInitialDirectory);

    //
    // Register the data editor window
    //

    RegisterDataEditor(hInstance);

    //
    // To make handles obtained by NtCreateFile usable for calling ReadFile and WriteFile,
    // we have to set the FILE_SYNCHRONOUS_IO_NONALERT into CreateOptions
    // and SYNCHRONIZE into DesiredAccess.
    //

    // Pre-load menus so they don't generate any FS requests when loaded
    memset(g_ContextMenus, 0, sizeof(g_ContextMenus));
    EnumResourceNames(g_hInst, RT_MENU, EnumMenusProc, NULL);

    // Set default values for opening relative file by NtOpenFile
    pData->dwDesiredAccessRF     = FILE_READ_DATA;
    pData->dwOpenOptionsRF       = 0;
    pData->dwShareAccessRF       = FILE_SHARE_READ | FILE_SHARE_WRITE;

    // Set default values for CreateFile and NtCreateFile
    pData->dwCreateDisposition1  = OPEN_ALWAYS;
    pData->dwCreateDisposition2  = FILE_OPEN_IF;
    pData->dwDesiredAccess       = dwDesiredAccess;
    pData->dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
    pData->dwShareAccess         = dwShareAccess;
    pData->dwCreateOptions       = dwCreateOptions;
    pData->dwObjAttrFlags        = OBJ_CASE_INSENSITIVE;
    pData->dwMoveFileFlags       = MOVEFILE_COPY_ALLOWED;
    pData->dwOplockLevel         = OPLOCK_LEVEL_CACHE_READ | OPLOCK_LEVEL_CACHE_WRITE;
    pData->dwCopyFileFlags       = dwCopyFileFlags;
    pData->dwMoveFileFlags       = dwMoveFileFlags;

    // Modify for synchronous open, if required
    if(bAsynchronousOpen == false)
    {
        pData->dwCreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;
        pData->dwDesiredAccess |= SYNCHRONIZE;
    }

    // Set default values for NtCreateSection/NtOpenSection
    pData->dwSectDesiredAccess   = SECTION_MAP_READ;
    pData->dwSectPageProtection  = PAGE_READONLY;
    pData->dwSectAllocAttributes = SEC_COMMIT;
    pData->dwSectWin32Protect    = PAGE_READONLY;

#ifdef _DEBUG
    DebugCode_TEST();
#endif

    // Call the dialog
    FileTestDialog(NULL, pData);

    // Free the data blobs
    pData->NtInfoData.Free();
    pData->RdWrData.Free();
    pData->OutData.Free();
    pData->InData.Free();

    // Cleanup the TFileTestData structure and exit
    if(pData->pFileEa != NULL)
        delete [] pData->pFileEa;
    HeapFree(g_hHeap, 0, pData);

    UnloadDynamicLoadedAPIs();
    return 0;
}
