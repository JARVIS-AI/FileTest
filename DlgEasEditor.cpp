/*****************************************************************************/
/* DlgEasEditor.cpp                       Copyright (c) Ladislav Zezula 2008 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 25.07.08  1.00  Lad  The first version of DlgEasEditor.cpp                */
/*****************************************************************************/

#include "FileTest.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// Local variables

static TListViewColumns Columns[] =
{
    {IDS_EA_NAME, 120},
    {IDS_EA_SIZE,  80},
    {IDS_EA_VALUE, -1},
    {0, 0}
};

//-----------------------------------------------------------------------------
// Local functions

static BOOL UpdateDialogButtons(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    HWND hButton;
    int nSelected = ListView_GetSelectedCount(hListView);
    int nItems = ListView_GetItemCount(hListView);
    int nItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    // "Insert will be always enabled"
    hButton = GetDlgItem(hDlg, IDC_INSERT);
    EnableWindow(hButton, TRUE);

    // Enable/disable "Edit"
    hButton = GetDlgItem(hDlg, IDC_EDIT);
    EnableWindow(hButton, (nSelected == 1));

    // Enable/disable "Delete"
    hButton = GetDlgItem(hDlg, IDC_DELETE);
    EnableWindow(hButton, (nSelected == 1));

    // Enable/disable "Up"
    hButton = GetDlgItem(hDlg, IDC_MOVE_UP);
    EnableWindow(hButton, (nItem > 0));

    // Enable/disable "Down"
    hButton = GetDlgItem(hDlg, IDC_MOVE_DOWN);
    EnableWindow(hButton, (0 <= nItem && nItem < (nItems - 1)));
    return TRUE;
}

static int SetListViewEaEntry(
    HWND hListView,
    int nIndex,
    PFILE_FULL_EA_INFORMATION pEaItem,
    BOOL bInsertNew)
{
    LVITEM lvi;
    LPTSTR szEaName;
    LPTSTR szEaValue;
    LPTSTR szTemp;
    PBYTE pbEaValue = (PBYTE)pEaItem->EaName + pEaItem->EaNameLength + 1;
    TCHAR szEaSize[20];
    size_t cchEaValue;
    int i;

    // -1 means insert to the end of the list
    if(nIndex == -1)
        nIndex = ListView_GetItemCount(hListView);

    // Allocate buffer for EA name and convert the name
    szEaName  = new TCHAR[pEaItem->EaNameLength + 1];
    for(i = 0; i < pEaItem->EaNameLength; i++)
        szEaName[i] = pEaItem->EaName[i];
    szEaName[i] = 0;

    // Allocate buffer for EA value and convert the value
    cchEaValue = (pEaItem->EaValueLength * 3) + 1;
    szEaValue = szTemp = new TCHAR[cchEaValue];
    if(szEaValue != NULL)
    {
        LPTSTR szEaValueEnd = szEaValue + cchEaValue;

        for(i = 0; i < pEaItem->EaValueLength; i++)
            StringCchPrintfEx(szTemp, (szEaValueEnd - szTemp), &szTemp, NULL, 0, _T("%02lX "), pbEaValue[i]);
        szTemp[0] = 0;

        // Set the main item
        ZeroMemory(&lvi, sizeof(LVITEM));
        lvi.mask    = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem   = nIndex;
        lvi.pszText = szEaName;
        lvi.lParam  = (LPARAM)pEaItem;

        // Insert or set the item
        if(bInsertNew)
            nIndex = ListView_InsertItem(hListView, &lvi);
        else
            ListView_SetItem(hListView, &lvi);

        // Set the sub item text
        StringCchPrintf(szEaSize, _countof(szEaSize), _T("0x%04X"), pEaItem->EaValueLength);
        ListView_SetItemText(hListView, nIndex, 1, szEaSize);

        // Set the sub item text
        ListView_SetItemText(hListView, nIndex, 2, szEaValue);

        delete [] szEaValue;
    }
    delete [] szEaName;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Dialog handlers

static int OnInitDialog(HWND hDlg, LPARAM lParam)
{
    TFileTestData * pData = (TFileTestData *)lParam;
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);

    // Save the user data
    SetDialogData(hDlg, lParam);
    CenterWindowToParent(hDlg);

    // Initialize the listview
    ListView_CreateColumns(hListView, Columns);
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    // Save the pointer to TFileTestData
    if(lParam != 0)
        ExtendedAttributesToListView(hDlg, pData->pFileEa);

    // Initialize the listview
    UpdateDialogButtons(hDlg);
    return TRUE;
}

void MoveItem(HWND hDlg, BOOL bUp)
{
    LVITEM lvi1;
    LVITEM lvi2;
    TCHAR szText1[MAX_PATH + 1] = _T("");
    TCHAR szText2[MAX_PATH + 1] = _T("");
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    int nItems = ListView_GetItemCount(hListView);
    int nItem1 = 0;
    int nItem2 = 0;

    if(bUp == FALSE)
    {
        nItem1 = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
        nItem2 = nItem1 + 1;
        if(nItem1 == nItems - 1)
            return;

    }
    else
    {
        nItem1 = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED) - 1;
        nItem2 = nItem1 + 1;
        if(nItem1 < 0)
            return;
    }

    // Disable redrawing during item acrobatics
    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

    // Retrieve the both items
    ZeroMemory(&lvi1, sizeof(LVITEM));
    lvi1.mask       = LVIF_IMAGE | LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
    lvi1.stateMask  = (UINT)-1;
    lvi1.iItem      = nItem1;
    lvi1.pszText    = szText1;
    lvi1.cchTextMax = _countof(szText1);
    ListView_GetItem(hListView, &lvi1);

    ZeroMemory(&lvi2, sizeof(LVITEM));
    lvi2.mask       = LVIF_IMAGE | LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
    lvi2.stateMask  = (UINT)-1;
    lvi2.iItem      = nItem2;
    lvi2.pszText    = szText2;
    lvi2.cchTextMax = _countof(szText2);
    ListView_GetItem(hListView, &lvi2);

    // Switch the main items
    lvi2.iItem = nItem1;
    ListView_SetItem(hListView, &lvi2);
    lvi1.iItem = nItem2;
    ListView_SetItem(hListView, &lvi1);

    // Switch the texts for the subitems
    ListView_GetItemText(hListView, nItem1, 1, szText1, _countof(szText1));
    ListView_GetItemText(hListView, nItem2, 1, szText2, _countof(szText2));
    ListView_SetItemText(hListView, nItem1, 1, szText2);
    ListView_SetItemText(hListView, nItem2, 1, szText1);

    // Enable redrawing and redraw the window
    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hListView, NULL, TRUE);
}

static int OnInsertEa(HWND hDlg)
{
    PFILE_FULL_EA_INFORMATION pEaItem = NULL;
    HWND hListView;

    // Call the dialog for creating new EA
    if(EaEditorDialog(hDlg, &pEaItem) != IDOK)
        return TRUE;

    // Insert listview
    hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    SetListViewEaEntry(hListView, -1, pEaItem, TRUE);
    UpdateDialogButtons(hDlg);
    return TRUE;
}

static int OnEditEa(HWND hDlg)
{
    PFILE_FULL_EA_INFORMATION pEaItem = NULL;
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    int nSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    // If no item selected, do nothing
    if(nSelected == -1)
        return 0;

    // Call the dialog for editing EA
    pEaItem = (PFILE_FULL_EA_INFORMATION)ListView_GetItemParam(hListView, nSelected);
    if(EaEditorDialog(hDlg, &pEaItem) != IDOK)
        return TRUE;
    
    SetListViewEaEntry(hListView, nSelected, pEaItem, FALSE);
    UpdateDialogButtons(hDlg);
    return TRUE;
}

static int OnDeleteEa(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    int nSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    // If no item selected, do nothing
    if(nSelected != -1)
        ListView_DeleteItem(hListView, nSelected);
    UpdateDialogButtons(hDlg);
    return TRUE;
}

               
static int OnDoubleClick(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    int nSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    if(nSelected == -1)
        OnInsertEa(hDlg);
    else
        OnEditEa(hDlg);
    return TRUE;
}


static int OnLVKeyDown(HWND hDlg, NMLVKEYDOWN * pLVKeyDown)
{
    switch(pLVKeyDown->wVKey)
    {
        case VK_INSERT:
            return OnInsertEa(hDlg);

        case VK_DELETE:
            return OnDeleteEa(hDlg);
    }
    return FALSE;
}

static BOOL OnSaveDialog(HWND hDlg)
{
    TFileTestData * pData = GetDialogData(hDlg);

    if(pData != NULL)
    {
        // Delete the existing EAs
        if(pData->pFileEa != NULL)
            delete [] pData->pFileEa;
        pData->pFileEa = NULL;
        pData->dwEaSize = 0;

        // Create new extended attributes
        pData->pFileEa = ListViewToExtendedAttributes(hDlg, pData->dwEaSize);
    }
    return TRUE;
}

static int OnDeleteItem(HWND /* hDlg */, LPNMLISTVIEW pNMListView)
{
    PFILE_FULL_EA_INFORMATION pEaItem = (PFILE_FULL_EA_INFORMATION)pNMListView->lParam;

    if(pEaItem != NULL)
        delete [] pEaItem;
    return TRUE;
}

static int OnCommand(HWND hDlg, UINT nNotify, UINT nIDCtrl)
{
    if(nNotify == BN_CLICKED)
    {
        switch(nIDCtrl)
        {
            case IDC_MOVE_UP:
                MoveItem(hDlg, TRUE);
                return TRUE;

            case IDC_MOVE_DOWN:
                MoveItem(hDlg, FALSE);
                return TRUE;

            case IDC_INSERT:
                OnInsertEa(hDlg);
                return TRUE;

            case IDC_EDIT:
                OnEditEa(hDlg);
                return TRUE;

            case IDC_DELETE:
                OnDeleteEa(hDlg);
                return TRUE;

            case IDOK:
                OnSaveDialog(hDlg);
                // No break here !!!

            case IDCANCEL:
                EndDialog(hDlg, nIDCtrl);
                return TRUE;
        }
    }

    return FALSE;
}

static int OnNotify(HWND hDlg, NMHDR * pNMHDR)
{
    switch(pNMHDR->code)
    {
        case NM_DBLCLK:
            if(pNMHDR->idFrom == IDC_EA_LIST)
                return OnDoubleClick(hDlg);
            break;

        case LVN_KEYDOWN:
            if(pNMHDR->idFrom == IDC_EA_LIST)
                return OnLVKeyDown(hDlg, (NMLVKEYDOWN *)pNMHDR);
            break;

        case LVN_ITEMCHANGED:
            if(pNMHDR->idFrom == IDC_EA_LIST)
                return UpdateDialogButtons(hDlg);
            break;

        case LVN_DELETEITEM:
            if(pNMHDR->idFrom == IDC_EA_LIST)
                return OnDeleteItem(hDlg, (LPNMLISTVIEW)pNMHDR);
            break;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Public functions

void ExtendedAttributesToListView(HWND hDlg, PFILE_FULL_EA_INFORMATION pFileEa)
{
    PFILE_FULL_EA_INFORMATION pEaItemCopy;
    PFILE_FULL_EA_INFORMATION pEaItem;
    ULONG dwEaItemSize = 0;
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);

    ListView_DeleteAllItems(hListView);

    // Load the listview with the extended attributes
    if(pFileEa != NULL)
    {
        pEaItem = pFileEa;
        for(;;)
        {
            // Create the EA copy and insert it to the list
            dwEaItemSize = GetEaEntrySize(pEaItem);
            pEaItemCopy = (PFILE_FULL_EA_INFORMATION)(new char [dwEaItemSize]);
            CopyMemory(pEaItemCopy, pEaItem, dwEaItemSize);
            pEaItemCopy->NextEntryOffset = dwEaItemSize;
            SetListViewEaEntry(hListView, -1, pEaItemCopy, TRUE);

            // Move to the next item
            if(pEaItem->NextEntryOffset == 0)
                break;
            pEaItem = (PFILE_FULL_EA_INFORMATION)((PBYTE)pEaItem + pEaItem->NextEntryOffset);
        }
    }
}

PFILE_FULL_EA_INFORMATION ListViewToExtendedAttributes(HWND hDlg, DWORD & dwOutEaLength)
{
    PFILE_FULL_EA_INFORMATION pEaItemCopy;
    PFILE_FULL_EA_INFORMATION pEaItem;
    PFILE_FULL_EA_INFORMATION pFileEa = NULL;
    DWORD dwEaLength = 0;
    HWND hListView = GetDlgItem(hDlg, IDC_EA_LIST);
    int nItemCount = ListView_GetItemCount(hListView);
    int nIndex;

    // Calculate the buffer necessary to set the EA.
    for(nIndex = 0; nIndex < nItemCount; nIndex++)
    {
        pEaItem = (PFILE_FULL_EA_INFORMATION)ListView_GetItemParam(hListView, nIndex);
        if(pEaItem != NULL)
        {
            dwEaLength += pEaItem->NextEntryOffset;
        }
    }

    // Allocate buffer for the complete EA list and copy the EAs to that buffer
    pEaItemCopy = pFileEa = (PFILE_FULL_EA_INFORMATION)(new BYTE[dwEaLength]);
    if(pFileEa != NULL)
    {
        for(nIndex = 0; nIndex < nItemCount; nIndex++)
        {
            pEaItem = (PFILE_FULL_EA_INFORMATION)ListView_GetItemParam(hListView, nIndex);
            if(pEaItem != NULL)
            {
                memcpy(pEaItemCopy, pEaItem, pEaItem->NextEntryOffset);

                // If this is the last item, we have to set zero as NextEntryOffset
                if(nIndex == nItemCount - 1)
                    pEaItemCopy->NextEntryOffset = 0;
                pEaItemCopy = (PFILE_FULL_EA_INFORMATION)((LPBYTE)pEaItemCopy + pEaItemCopy->NextEntryOffset);
            }
        }
    }
    else
    {
        dwEaLength = 0;
    }

    // Give the EAs tothe caller
    dwOutEaLength = dwEaLength;
    return pFileEa;
}

// Common dialog proc, shared by Page08Ea.cpp
// - Returns nonzero = message has been handled
// - Returns zero = message has NOT been handled
INT_PTR CALLBACK ExtendedAttributesEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hDlg, lParam);

        case WM_COMMAND:
            return OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam));

        case WM_NOTIFY:
            return OnNotify(hDlg, (NMHDR *)lParam);
    }

    return FALSE;
}

INT_PTR ExtendedAtributesEditorDialog(HWND hParent, TFileTestData * pData)
{
    return DialogBoxParam(g_hInst,
                          MAKEINTRESOURCE(IDD_EAS_EDITOR),
                          hParent,
                          ExtendedAttributesEditorProc,
                  (LPARAM)pData);
}
