/*
TMPCLASS.CPP

Temporary panel plugin class implementation

*/

#include "TmpPanel.hpp"

TmpPanel::TmpPanel(const wchar_t* pHostFile)
{
	LastOwnersRead = FALSE;
	LastGroupsRead = FALSE;
	LastLinksRead = FALSE;
	UpdateNotNeeded = FALSE;
	TmpPanelItem = NULL;
	TmpItemsNumber = 0;
	PanelIndex = CurrentCommonPanel;
	IfOptCommonPanel();


	HostFile=nullptr;
	if (pHostFile)
	{
		HostFile = (wchar_t*)malloc((wcslen(pHostFile)+1)*sizeof(wchar_t));
		wcscpy(HostFile, pHostFile);
	}

}

TmpPanel::~TmpPanel()
{
	if (!StartupOptCommonPanel)
		FreePanelItems(TmpPanelItem, TmpItemsNumber);

	if (HostFile)
		free(HostFile);
}

int TmpPanel::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	IfOptCommonPanel();

	int Size = Info.Control(this, FCTL_GETCOLUMNTYPES, 0, 0);
	wchar_t *ColumnTypes = new wchar_t[Size];
	Info.Control(this, FCTL_GETCOLUMNTYPES, Size, (LONG_PTR)ColumnTypes);
	UpdateItems(IsOwnersDisplayed(ColumnTypes), IsGroupsDisplayed(ColumnTypes), IsLinksDisplayed(ColumnTypes));
	delete[] ColumnTypes;
	*pPanelItem = TmpPanelItem;
	*pItemsNumber = TmpItemsNumber;

	return (TRUE);
}

void TmpPanel::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags =
			OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWNAMESONLY;

	if (!Opt.SafeModePanel)
		Info->Flags|= OPIF_REALNAMES;

	Info->HostFile=this->HostFile;
	Info->CurDir = L"";

	Info->Format = (wchar_t *)GetMsg(MTempPanel);

	static wchar_t Title[100] = {};
#define PANEL_MODE (Opt.SafeModePanel ? L"(R) " : L"")
	if (StartupOptCommonPanel)
		FSF.snprintf(Title, ARRAYSIZE(Title) - 1, GetMsg(MTempPanelTitleNum), PANEL_MODE, PanelIndex);
	else
		FSF.snprintf(Title, ARRAYSIZE(Title) - 1, L" %ls%ls ", PANEL_MODE, GetMsg(MTempPanel));
#undef PANEL_MODE

	Info->PanelTitle = Title;

	static struct PanelMode PanelModesArray[10];
	PanelModesArray[4].FullScreen =
			(StartupOpenFrom == OPEN_COMMANDLINE) ? Opt.FullScreenPanel : StartupOptFullScreenPanel;
	PanelModesArray[4].ColumnTypes = Opt.ColumnTypes;
	PanelModesArray[4].ColumnWidths = Opt.ColumnWidths;
	PanelModesArray[4].StatusColumnTypes = Opt.StatusColumnTypes;
	PanelModesArray[4].StatusColumnWidths = Opt.StatusColumnWidths;
	PanelModesArray[4].CaseConversion = TRUE;

	Info->PanelModesArray = PanelModesArray;
	Info->PanelModesNumber = ARRAYSIZE(PanelModesArray);
	Info->StartPanelMode = L'4';
	static struct KeyBarTitles KeyBar;
	memset(&KeyBar, 0, sizeof(KeyBar));
	KeyBar.Titles[7 - 1] = (wchar_t *)GetMsg(MF7);
	if (StartupOptCommonPanel)
		KeyBar.AltShiftTitles[12 - 1] = (wchar_t *)GetMsg(MAltShiftF12);
	KeyBar.AltShiftTitles[2 - 1] = (wchar_t *)GetMsg(MAltShiftF2);
	KeyBar.AltShiftTitles[3 - 1] = (wchar_t *)GetMsg(MAltShiftF3);
	Info->KeyBar = &KeyBar;
}

int TmpPanel::SetDirectory(const wchar_t *Dir, int OpMode)
{
	if ((OpMode & OPM_FIND) /* || wcscmp(Dir,L"\\")==0*/)
		return (FALSE);
	if (wcscmp(Dir, WGOOD_SLASH) == 0)
		Info.Control(this, FCTL_CLOSEPLUGIN, 0, 0);
	else
		Info.Control(this, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)Dir);

    return (TRUE);
}

int TmpPanel::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int, const wchar_t *SrcPath, int)
{
	UpdateNotNeeded = FALSE;

	HANDLE hScreen = BeginPutFiles();
	for (int i = 0; i < ItemsNumber; i++) {
		if (!PutOneFile(SrcPath, PanelItem[i])) {
			CommitPutFiles(hScreen, FALSE);
			return FALSE;
		}
	}
	CommitPutFiles(hScreen, TRUE);

	return (1);
}

HANDLE TmpPanel::BeginPutFiles()
{
	IfOptCommonPanel();
	Opt.SelectedCopyContents = Opt.CopyContents;

	HANDLE hScreen = Info.SaveScreen(0, 0, -1, -1);
	const wchar_t *MsgItems[] = {GetMsg(MTempPanel), GetMsg(MTempSendFiles)};
	Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 0);
	return hScreen;
}

static inline int cmp_names(const WIN32_FIND_DATA &wfd, const FAR_FIND_DATA &ffd)
{
	return wcscmp(wfd.cFileName, FSF.PointToName(ffd.lpwszFileName));
}

int TmpPanel::PutDirectoryContents(const wchar_t *Path)
{
	if (Opt.SelectedCopyContents == 2) {
		const wchar_t *MsgItems[] = {GetMsg(MWarning), GetMsg(MCopyContentsMsg)};
		Opt.SelectedCopyContents = !Info.Message(Info.ModuleNumber, FMSG_MB_YESNO, L"Config", MsgItems,
				ARRAYSIZE(MsgItems), 0);
	}
	if (Opt.SelectedCopyContents) {
		FAR_FIND_DATA *DirItems;

		int DirItemsNumber;
		if (!Info.GetDirList(Path, &DirItems, &DirItemsNumber)) {
			FreePanelItems(TmpPanelItem, TmpItemsNumber);
			TmpItemsNumber = 0;
			return FALSE;
		}
		struct PluginPanelItem *NewPanelItem = (struct PluginPanelItem *)realloc(TmpPanelItem,
				sizeof(*TmpPanelItem) * (TmpItemsNumber + DirItemsNumber));
		if (NewPanelItem == NULL)
			return FALSE;
		TmpPanelItem = NewPanelItem;
		memset(&TmpPanelItem[TmpItemsNumber], 0, sizeof(*TmpPanelItem) * DirItemsNumber);

		for (int i = 0; i < DirItemsNumber; i++) {
			struct PluginPanelItem *CurPanelItem = &TmpPanelItem[TmpItemsNumber];
			CurPanelItem->UserData = TmpItemsNumber;
			TmpItemsNumber++;

			CurPanelItem->FindData = DirItems[i];
			CurPanelItem->FindData.lpwszFileName = wcsdup(DirItems[i].lpwszFileName);
		}
		Info.FreeDirList(DirItems, DirItemsNumber);
	}
	return TRUE;
}

int TmpPanel::PutOneFile(const wchar_t *SrcPath, PluginPanelItem &PanelItem)
{
	struct PluginPanelItem *NewPanelItem =
			(struct PluginPanelItem *)realloc(TmpPanelItem, sizeof(*TmpPanelItem) * (TmpItemsNumber + 1));
	if (NewPanelItem == NULL)
		return FALSE;
	TmpPanelItem = NewPanelItem;
	struct PluginPanelItem *CurPanelItem = &TmpPanelItem[TmpItemsNumber];
	memset(CurPanelItem, 0, sizeof(*CurPanelItem));
	CurPanelItem->FindData = PanelItem.FindData;
	CurPanelItem->UserData = TmpItemsNumber;
	CurPanelItem->FindData.lpwszFileName = reinterpret_cast<wchar_t *>(
			malloc((wcslen(SrcPath) + 1 + wcslen(PanelItem.FindData.lpwszFileName) + 1) * sizeof(wchar_t)));
	if (CurPanelItem->FindData.lpwszFileName == NULL)
		return FALSE;

	*(wchar_t*)CurPanelItem->FindData.lpwszFileName = L'\0';
	if (*SrcPath && !wcschr(PanelItem.FindData.lpwszFileName, L'/')) {
		wcscpy((wchar_t *)CurPanelItem->FindData.lpwszFileName, SrcPath);
		FSF.AddEndSlash((wchar_t *)CurPanelItem->FindData.lpwszFileName);
	}

	wcscat((wchar_t *)CurPanelItem->FindData.lpwszFileName, PanelItem.FindData.lpwszFileName);
	TmpItemsNumber++;
	if (Opt.SelectedCopyContents && (CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		return PutDirectoryContents(CurPanelItem->FindData.lpwszFileName);
	return TRUE;
}

int TmpPanel::PutOneFile(const wchar_t *FilePath)
{
	struct PluginPanelItem *NewPanelItem =
			(struct PluginPanelItem *)realloc(TmpPanelItem, sizeof(*TmpPanelItem) * (TmpItemsNumber + 1));
	if (NewPanelItem == NULL)
		return FALSE;
	TmpPanelItem = NewPanelItem;
	struct PluginPanelItem *CurPanelItem = &TmpPanelItem[TmpItemsNumber];
	memset(CurPanelItem, 0, sizeof(*CurPanelItem));
	CurPanelItem->UserData = TmpItemsNumber;
	if (GetFileInfoAndValidate(FilePath, &CurPanelItem->FindData, Opt.AnyInPanel)) {
		TmpItemsNumber++;
		if (Opt.SelectedCopyContents && (CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return PutDirectoryContents(CurPanelItem->FindData.lpwszFileName);
	}
	return TRUE;
}

void TmpPanel::CommitPutFiles(HANDLE hRestoreScreen, int Success)
{
	if (Success)
		RemoveDups();
	Info.RestoreScreen(hRestoreScreen);
}

int TmpPanel::SetFindList(const struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	HANDLE hScreen = BeginPutFiles();
	FindSearchResultsPanel();
	FreePanelItems(TmpPanelItem, TmpItemsNumber);
	TmpItemsNumber = 0;
	TmpPanelItem = (PluginPanelItem *)malloc(sizeof(PluginPanelItem) * ItemsNumber);
	if (TmpPanelItem) {
		TmpItemsNumber = ItemsNumber;
		memset(TmpPanelItem, 0, TmpItemsNumber * sizeof(*TmpPanelItem));
		for (int i = 0; i < ItemsNumber; ++i) {
			TmpPanelItem[i].UserData = i;
			TmpPanelItem[i].FindData = PanelItem[i].FindData;
			if (TmpPanelItem[i].FindData.lpwszFileName)
				TmpPanelItem[i].FindData.lpwszFileName = wcsdup(TmpPanelItem[i].FindData.lpwszFileName);
		}
	}
	CommitPutFiles(hScreen, TRUE);
	UpdateNotNeeded = TRUE;
	return (TRUE);
}

void TmpPanel::FindSearchResultsPanel()
{
	if (StartupOptCommonPanel) {
		if (!Opt.NewPanelForSearchResults)
			IfOptCommonPanel();
		else {
			int SearchResultsPanel = -1;
			for (int i = 0; i < COMMONPANELSNUMBER; i++) {
				if (CommonPanels[i].ItemsNumber == 0) {
					SearchResultsPanel = i;
					break;
				}
			}
			if (SearchResultsPanel < 0) {
				// all panels are full - use least recently used panel
				SearchResultsPanel = Opt.LastSearchResultsPanel++;
				if (Opt.LastSearchResultsPanel >= COMMONPANELSNUMBER)
					Opt.LastSearchResultsPanel = 0;
			}
			if (PanelIndex != SearchResultsPanel) {
				CommonPanels[PanelIndex].Items = TmpPanelItem;
				CommonPanels[PanelIndex].ItemsNumber = TmpItemsNumber;
				PanelIndex = SearchResultsPanel;
				TmpPanelItem = CommonPanels[PanelIndex].Items;
				TmpItemsNumber = CommonPanels[PanelIndex].ItemsNumber;
			}
			CurrentCommonPanel = PanelIndex;
		}
	}
}

int _cdecl SortListCmp(const void *el1, const void *el2, void *userparam)
{
	PluginPanelItem *TmpPanelItem = reinterpret_cast<PluginPanelItem *>(userparam);
	int idx1 = *reinterpret_cast<const int *>(el1);
	int idx2 = *reinterpret_cast<const int *>(el2);
	int res = wcscmp(TmpPanelItem[idx1].FindData.lpwszFileName, TmpPanelItem[idx2].FindData.lpwszFileName);
	if (res == 0) {
		if (idx1 < idx2)
			return -1;
		else if (idx1 == idx2)
			return 0;
		else
			return 1;
	} else
		return res;
}

void TmpPanel::RemoveDups()
{
	int *indices = reinterpret_cast<int *>(malloc(TmpItemsNumber * sizeof(int)));
	if (indices == NULL)
		return;
	for (int i = 0; i < TmpItemsNumber; i++)
		indices[i] = i;
	FSF.qsortex(indices, TmpItemsNumber, sizeof(int), SortListCmp, TmpPanelItem);
	for (int i = 0; i + 1 < TmpItemsNumber; i++)
		if (wcscmp(TmpPanelItem[indices[i]].FindData.lpwszFileName,
					TmpPanelItem[indices[i + 1]].FindData.lpwszFileName)
				== 0)
			TmpPanelItem[indices[i + 1]].Flags|= REMOVE_FLAG;
	free(indices);
	RemoveEmptyItems();
}

void TmpPanel::RemoveEmptyItems()
{
	int EmptyCount = 0;
	struct PluginPanelItem *CurItem = TmpPanelItem;
	for (int i = 0; i < TmpItemsNumber; i++, CurItem++)
		if (CurItem->Flags & REMOVE_FLAG) {
			if (CurItem->Owner) {
				free((void *)CurItem->Owner);
				CurItem->Owner = NULL;
			}
			if (CurItem->Group) {
				free((void *)CurItem->Group);
				CurItem->Group = NULL;
			}
			if (CurItem->FindData.lpwszFileName) {
				free((wchar_t *)CurItem->FindData.lpwszFileName);
				CurItem->FindData.lpwszFileName = NULL;
			}
			EmptyCount++;
		} else if (EmptyCount) {
			CurItem->UserData-= EmptyCount;
			*(CurItem - EmptyCount) = *CurItem;
			memset(CurItem, 0, sizeof(*CurItem));
		}

	TmpItemsNumber-= EmptyCount;
	if (EmptyCount > 1)
		TmpPanelItem =
				(struct PluginPanelItem *)realloc(TmpPanelItem, sizeof(*TmpPanelItem) * (TmpItemsNumber + 1));

	if (StartupOptCommonPanel) {
		CommonPanels[PanelIndex].Items = TmpPanelItem;
		CommonPanels[PanelIndex].ItemsNumber = TmpItemsNumber;
	}
}

void TmpPanel::UpdateItems(int ShowOwners, int ShowGroups, int ShowLinks)
{
	if (UpdateNotNeeded || TmpItemsNumber == 0) {
		UpdateNotNeeded = FALSE;
		return;
	}
	HANDLE hScreen = Info.SaveScreen(0, 0, -1, -1);
	const wchar_t *MsgItems[] = {GetMsg(MTempPanel), GetMsg(MTempUpdate)};
	Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 0);
	LastOwnersRead = ShowOwners;
	LastGroupsRead = ShowGroups;
	LastLinksRead = ShowLinks;
	struct PluginPanelItem *CurItem = TmpPanelItem;

	for (int i = 0; i < TmpItemsNumber; i++, CurItem++) {
		HANDLE FindHandle;
		const wchar_t *lpFullName = CurItem->FindData.lpwszFileName;

		const wchar_t *lpSlash = wcsrchr(lpFullName, GOOD_SLASH);
		int Length = lpSlash ? (int)(lpSlash - lpFullName + 1) : 0;

		int SameFolderItems = 1;
		/* $ 23.12.2001 DJ
		   если FullName - это каталог, то FindFirstFile (FullName+"*.*")
		   этот каталог не найдет. Поэтому для каталогов оптимизацию с
		   SameFolderItems пропускаем.
		*/
		if (Length > 0 && Length > (int)wcslen(lpFullName))	/* DJ $ */
		{
			for (int j = 1; i + j < TmpItemsNumber; j++) {
				if (memcmp(lpFullName, CurItem[j].FindData.lpwszFileName, Length * sizeof(wchar_t)) == 0
						&& wcschr((const wchar_t *)CurItem[j].FindData.lpwszFileName + Length, GOOD_SLASH)
								== NULL) {
					SameFolderItems++;
				} else {
					break;
				}
			}
		}

		// SameFolderItems - оптимизация для случая, когда в панели лежат
		// несколько файлов из одного и того же каталога. При этом
		// FindFirstFile() делается один раз на каталог, а не отдельно для
		// каждого файла.
		if (SameFolderItems > 2) {
			WIN32_FIND_DATA FindData;

			StrBuf FindFile((int)(lpSlash - lpFullName) + 1 + 1 + 1);
			wcsncpy(FindFile, lpFullName, (int)(lpSlash - lpFullName) + 1);
			wcscpy(FindFile + (lpSlash + 1 - lpFullName), L"*");

			for (int J = 0; J < SameFolderItems; J++)
				CurItem[J].Flags|= REMOVE_FLAG;

			int Done = (FindHandle = FindFirstFile(FindFile, &FindData)) == INVALID_HANDLE_VALUE;
			while (!Done) {
				for (int J = 0; J < SameFolderItems; J++) {
					if ((CurItem[J].Flags & 1) && cmp_names(FindData, CurItem[J].FindData) == 0) {
						CurItem[J].Flags&= ~REMOVE_FLAG;

						const wchar_t *save = CurItem[J].FindData.lpwszFileName;
						WFD2FFD(FindData, CurItem[J].FindData);

						free((wchar_t *)CurItem[J].FindData.lpwszFileName);
						CurItem[J].FindData.lpwszFileName = save;
						break;
					}
				}

				Done = !FindNextFile(FindHandle, &FindData);
			}
			FindClose(FindHandle);
			i+= SameFolderItems - 1;
			CurItem+= SameFolderItems - 1;
		} else {
			if (!GetFileInfoAndValidate(lpFullName, &CurItem->FindData, Opt.AnyInPanel))
				CurItem->Flags|= REMOVE_FLAG;
		}
	}

	RemoveEmptyItems();

	if (ShowOwners || ShowGroups || ShowLinks) {
		struct PluginPanelItem *CurItem = TmpPanelItem;
		for (int i = 0; i < TmpItemsNumber; i++, CurItem++) {
			if (ShowOwners) {
				wchar_t Owner[80];
				if (CurItem->Owner) {
					free((void *)CurItem->Owner);
					CurItem->Owner = NULL;
				}
				if (FSF.GetFileOwner(NULL, CurItem->FindData.lpwszFileName, Owner,80))
					CurItem->Owner = wcsdup(Owner);
			}
			if (ShowGroups) {
				wchar_t Group[80];
				if (CurItem->Group) {
					free((void *)CurItem->Group);
					CurItem->Group = NULL;
				}
				if (FSF.GetFileGroup(NULL, CurItem->FindData.lpwszFileName, Group,80))
					CurItem->Group = wcsdup(Group);
			}
			if (ShowLinks)
				CurItem->NumberOfLinks = FSF.GetNumberOfLinks(CurItem->FindData.lpwszFileName);
		}
	}
	Info.RestoreScreen(hScreen);
}

int TmpPanel::ProcessEvent(int Event, void *)
{
	if (Event == FE_CHANGEVIEWMODE) {
		IfOptCommonPanel();

		int Size = Info.Control(this, FCTL_GETCOLUMNTYPES, 0, 0);
		wchar_t *ColumnTypes = new wchar_t[Size];
		Info.Control(this, FCTL_GETCOLUMNTYPES, Size, (LONG_PTR)ColumnTypes);
		int UpdateOwners = IsOwnersDisplayed(ColumnTypes) && !LastOwnersRead;
		int UpdateGroups = IsGroupsDisplayed(ColumnTypes) && !LastGroupsRead;
		int UpdateLinks = IsLinksDisplayed(ColumnTypes) && !LastLinksRead;
		delete[] ColumnTypes;


		if (UpdateOwners || UpdateGroups || UpdateLinks) {
			UpdateItems(UpdateOwners, UpdateGroups, UpdateLinks);

			Info.Control(this, FCTL_UPDATEPANEL, TRUE, 0);
			Info.Control(this, FCTL_REDRAWPANEL, 0, 0);
		}
	}
	return (FALSE);
}

bool TmpPanel::IsCurrentFileCorrect(wchar_t **pCurFileName)
{
	struct PanelInfo PInfo;
	const wchar_t *CurFileName = NULL;

	if (pCurFileName)
		*pCurFileName = NULL;

	Info.Control(this, FCTL_GETPANELINFO, 0, (LONG_PTR)&PInfo);
	PluginPanelItem *PPI =
			(PluginPanelItem *)malloc(Info.Control(this, FCTL_GETPANELITEM, PInfo.CurrentItem, 0));
	if (PPI) {
		Info.Control(this, FCTL_GETPANELITEM, PInfo.CurrentItem, (LONG_PTR)PPI);
		CurFileName = PPI->FindData.lpwszFileName;
	} else {
		return false;
	}

	bool IsCorrectFile = false;
	if (wcscmp(CurFileName, L"..") == 0) {
		IsCorrectFile = true;
	} else {
		FAR_FIND_DATA TempFindData = {};
		IsCorrectFile = GetFileInfoAndValidate(CurFileName, &TempFindData, FALSE);
		if (TempFindData.lpwszFileName) {
			free((void *)TempFindData.lpwszFileName);
		}
	}

	if (pCurFileName) {
		*pCurFileName = (wchar_t *)malloc((wcslen(CurFileName) + 1) * sizeof(wchar_t));
		wcscpy(*pCurFileName, CurFileName);
	}

	free(PPI);

	return IsCorrectFile;
}

int TmpPanel::ProcessKey(int Key, unsigned int ControlState)
{
	if (!ControlState && Key == VK_F1) {
		Info.ShowHelp(Info.ModuleName, NULL, FHELP_USECONTENTS | FHELP_NOSHOWERROR);
		return TRUE;
	}

	if (ControlState == (PKF_SHIFT | PKF_ALT) && Key == VK_F9) {
		EXP_NAME(Configure)(0);
		return TRUE;
	}

	if (ControlState == (PKF_SHIFT | PKF_ALT) && Key == VK_F3) {
		PtrGuard CurFileName;
		if (IsCurrentFileCorrect(CurFileName.PtrPtr())) {
			struct PanelInfo PInfo;
			Info.Control(this, FCTL_GETPANELINFO, 0, (LONG_PTR)&PInfo);
			if (wcscmp(CurFileName, L"..") != 0) {

				PluginPanelItem *PPI = (PluginPanelItem *)malloc(
						Info.Control(this, FCTL_GETPANELITEM, PInfo.CurrentItem, 0));
				DWORD attributes = 0;
				if (PPI) {
					Info.Control(this, FCTL_GETPANELITEM, PInfo.CurrentItem, (LONG_PTR)PPI);
					attributes = PPI->FindData.dwFileAttributes;
					free(PPI);
				}

				if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
					Info.Control(PANEL_PASSIVE, FCTL_SETPANELDIR, 0, (LONG_PTR)CurFileName.Ptr());
				} else {
					GoToFile(CurFileName, true);
				}

				Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

				return (TRUE);
			}
		}
	}

	if (ControlState != PKF_CONTROL && Key >= VK_F3 && Key <= VK_F8 && Key != VK_F7) {
		if (!IsCurrentFileCorrect(NULL))
			return (TRUE);
	}

	if (ControlState == 0 && Key == VK_RETURN && Opt.AnyInPanel) {
		PtrGuard CurFileName;
		if (!IsCurrentFileCorrect(CurFileName.PtrPtr())) {
			Info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)CurFileName.Ptr());
			return (TRUE);
		}
	}

	if (ControlState == PKF_CONTROL && Key == VK_PRIOR) {
		PtrGuard CurFileName;
		if (IsCurrentFileCorrect(CurFileName.PtrPtr())) {
			if (wcscmp(CurFileName, L"..")) {
				GoToFile(CurFileName, false);
				return TRUE;
			}
		}

		if (CurFileName.Ptr() && !wcscmp(CurFileName, L"..")) {
			SetDirectory(L".", 0);
			return TRUE;
		}
	}

	if (ControlState == 0 && Key == VK_F7) {
		ProcessRemoveKey();
		return TRUE;
	} else if (ControlState == (PKF_SHIFT | PKF_ALT) && Key == VK_F2) {
		ProcessSaveListKey();
		return TRUE;
	} else {
		if (StartupOptCommonPanel && ControlState == (PKF_SHIFT | PKF_ALT)) {
			if (Key == VK_F12) {
				ProcessPanelSwitchMenu();
				return (TRUE);
			} else if (Key >= L'0' && Key <= L'9') {
				SwitchToPanel(Key - L'0');
				return TRUE;
			}
		}
	}
	return (FALSE);
}

void TmpPanel::ProcessRemoveKey()
{
	IfOptCommonPanel();

	struct PanelInfo PInfo;

	Info.Control(this, FCTL_GETPANELINFO, 0, (LONG_PTR)&PInfo);
	for (int i = 0; i < PInfo.SelectedItemsNumber; i++) {
		struct PluginPanelItem *RemovedItem = NULL;
		PluginPanelItem *PPI = (PluginPanelItem *)malloc(Info.Control(this, FCTL_GETSELECTEDPANELITEM, i, 0));
		if (PPI) {
			Info.Control(this, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)PPI);
			RemovedItem = TmpPanelItem + PPI->UserData;
		}
		if (RemovedItem != NULL)
			RemovedItem->Flags|= REMOVE_FLAG;
		free(PPI);
	}
	RemoveEmptyItems();

	Info.Control(this, FCTL_UPDATEPANEL, 0, 0);
	Info.Control(this, FCTL_REDRAWPANEL, 0, 0);
	Info.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PInfo);

	if (PInfo.PanelType == PTYPE_QVIEWPANEL) {
		Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
	}
}

void TmpPanel::ProcessSaveListKey()
{
	IfOptCommonPanel();
	if (TmpItemsNumber == 0)
		return;

	// default path: opposite panel directory\panel<index>.<mask extension>
	StrBuf ListPath(NT_MAX_PATH + 20 + 512);
	Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, NT_MAX_PATH, (LONG_PTR)ListPath.Ptr());

	FSF.AddEndSlash(ListPath);
	wcscat(ListPath, L"panel");
	if (Opt.CommonPanel)
		FSF.itoa(PanelIndex, ListPath.Ptr() + wcslen(ListPath), 10);

	wchar_t ExtBuf[512];
	wcscpy(ExtBuf, Opt.Mask);
	wchar_t *comma = wcschr(ExtBuf, L',');
	if (comma)
		*comma = L'\0';
	wchar_t *ext = wcschr(ExtBuf, L'.');
	if (ext && !wcschr(ext, L'*') && !wcschr(ext, L'?'))
		wcscat(ListPath, ext);

	if (Info.InputBox(GetMsg(MTempPanel), GetMsg(MListFilePath), L"TmpPanel.SaveList", ListPath, ListPath,
				ListPath.Size() - 1, NULL, FIB_BUTTONS)) {
		SaveListFile(ListPath);
		Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
	}
}

void TmpPanel::SaveListFile(const wchar_t *Path)
{
	IfOptCommonPanel();

	if (!TmpItemsNumber)
		return;

	StrBuf FullPath;
	GetFullPath(Path, FullPath);

	HANDLE hFile = CreateFile(FullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		const wchar_t *Items[] = {GetMsg(MError)};
		Info.Message(Info.ModuleNumber, FMSG_WARNING | FMSG_ERRORTYPE | FMSG_MB_OK, NULL, Items, 1, 0);
		return;
	}

	DWORD BytesWritten;

	static const char bom_utf8[] = {'\xEF', '\xBB', '\xBF'};
	WriteFile(hFile, &bom_utf8, sizeof(bom_utf8), &BytesWritten, NULL);

	int i = 0;
	do {
		static const char *LF = "\n";
		const wchar_t *FName = TmpPanelItem[i].FindData.lpwszFileName;

	    size_t Size = 4 * wcslen(FName) + 1;
	    LPSTR FNameA = new char[Size];
	    PWZ_to_PZ(FName, FNameA, Size);

		WriteFile(hFile, FNameA, sizeof(char) * strlen(FNameA), &BytesWritten, NULL);
		WriteFile(hFile, LF, sizeof(char) * strlen(LF), &BytesWritten, NULL);

	    delete[] FNameA;
	} while (++i < TmpItemsNumber);
	CloseHandle(hFile);
}

void TmpPanel::SwitchToPanel(int NewPanelIndex)
{
	if ((unsigned)NewPanelIndex < COMMONPANELSNUMBER && NewPanelIndex != (int)PanelIndex) {
		CommonPanels[PanelIndex].Items = TmpPanelItem;
		CommonPanels[PanelIndex].ItemsNumber = TmpItemsNumber;
		if (!CommonPanels[NewPanelIndex].Items) {
			CommonPanels[NewPanelIndex].ItemsNumber = 0;
			CommonPanels[NewPanelIndex].Items = (PluginPanelItem *)calloc(1, sizeof(PluginPanelItem));
		}
		if (CommonPanels[NewPanelIndex].Items) {
			CurrentCommonPanel = PanelIndex = NewPanelIndex;
			Info.Control(this, FCTL_UPDATEPANEL, 0, 0);
			Info.Control(this, FCTL_REDRAWPANEL, 0, 0);
		}
	}
}

void TmpPanel::ProcessPanelSwitchMenu()
{
	FarMenuItem fmi[COMMONPANELSNUMBER];
	memset(&fmi, 0, sizeof(FarMenuItem) * COMMONPANELSNUMBER);
	const wchar_t *txt = GetMsg(MSwitchMenuTxt);

	wchar_t tmpstr[COMMONPANELSNUMBER][128];
	static const wchar_t fmt1[] = L"&%c. %ls %d";
	for (unsigned int i = 0; i < COMMONPANELSNUMBER; ++i) {

#define _OUT tmpstr[i]
		fmi[i].Text = tmpstr[i];
		if (i < 10)
			FSF.snprintf(_OUT, sizeof(_OUT) - 1, fmt1, L'0' + i, txt, CommonPanels[i].ItemsNumber);
		else if (i < 36)
			FSF.snprintf(_OUT, sizeof(_OUT) - 1, fmt1, L'A' - 10 + i, txt, CommonPanels[i].ItemsNumber);
		else
			FSF.snprintf(_OUT, sizeof(_OUT) - 1, L"   %ls %d", txt, CommonPanels[i].ItemsNumber);
#undef _OUT
	}
	fmi[PanelIndex].Selected = TRUE;
	int ExitCode = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE,
			GetMsg(MSwitchMenuTitle), NULL, NULL, NULL, NULL, fmi, COMMONPANELSNUMBER);
	SwitchToPanel(ExitCode);
}

int TmpPanel::IsOwnersDisplayed(LPCTSTR ColumnTypes)
{
	for (int i = 0; ColumnTypes[i]; i++)
		if (ColumnTypes[i] == L'O' && (i == 0 || ColumnTypes[i - 1] == L',')
				&& (ColumnTypes[i + 1] == L',' || ColumnTypes[i + 1] == 0))
			return (TRUE);
	return (FALSE);
}

int TmpPanel::IsGroupsDisplayed(LPCTSTR ColumnTypes)
{
	for (int i = 0; ColumnTypes[i]; i++)
		if (ColumnTypes[i] == L'U' && (i == 0 || ColumnTypes[i - 1] == L',')
				&& (ColumnTypes[i + 1] == L',' || ColumnTypes[i + 1] == 0))
			return (TRUE);
	return (FALSE);
}

int TmpPanel::IsLinksDisplayed(LPCTSTR ColumnTypes)
{
	for (int i = 0; ColumnTypes[i]; i++)
		if (ColumnTypes[i] == L'L' && ColumnTypes[i + 1] == L'N'
				&& (i == 0 || ColumnTypes[i - 1] == L',')
				&& (ColumnTypes[i + 2] == L',' || ColumnTypes[i + 2] == 0))
			return (TRUE);
	return (FALSE);
}

bool TmpPanel::GetFileInfoAndValidate(const wchar_t *FilePath, FAR_FIND_DATA *FindData, int Any)
{
	StrBuf ExpFilePath;
	ExpandEnvStrs(FilePath, ExpFilePath);

	wchar_t *FileName = ExpFilePath;
	ParseParam(FileName);

	StrBuf FullPath;
	GetFullPath(FileName, FullPath);

	bool Result = false;

	if (!wcscmp(FileName, L"/")) {
		FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		Result = true;
	} else {
		if (wcslen(FileName)) {
			DWORD dwAttr = GetFileAttributes(FullPath);
			if (dwAttr != INVALID_FILE_ATTRIBUTES) {
				WIN32_FIND_DATA wfd;
				HANDLE fff = FindFirstFile(FullPath, &wfd);
				if (fff != INVALID_HANDLE_VALUE) {
					FindClose(fff);
				} else {
					memset(&wfd, 0, sizeof(wfd));
					wfd.dwFileAttributes = dwAttr;
					HANDLE hFile = CreateFile(FullPath, FILE_READ_ATTRIBUTES,
											  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
											  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
					if (hFile != INVALID_HANDLE_VALUE) {
						GetFileTime(hFile, &wfd.ftCreationTime, &wfd.ftLastAccessTime, &wfd.ftLastWriteTime);
						wfd.nPhysicalSize = wfd.nFileSize = GetFileSize64(hFile);
						CloseHandle(hFile);
					}
				}
				WFD2FFD(wfd, *FindData);
				FileName = FullPath;
				Result = true;
			} else {
				if (Any) {
					FindData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
					Result = true;
				}
			}
		}
	}

	if (Result) {
		if (FindData->lpwszFileName)
			free((void *)FindData->lpwszFileName);
		FindData->lpwszFileName = wcsdup(FileName);
	}

	return Result;
}

void TmpPanel::IfOptCommonPanel(void)
{
	if (StartupOptCommonPanel) {
		TmpPanelItem = CommonPanels[PanelIndex].Items;
		TmpItemsNumber = CommonPanels[PanelIndex].ItemsNumber;
	}
}
