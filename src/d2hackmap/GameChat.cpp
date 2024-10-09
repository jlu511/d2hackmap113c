#include "stdafx.h"
#include "d2ptrs.h"

D2EditBox*	pD2WinEditBox=NULL;
ToggleVar 	tWisperNotice={				TOGGLEVAR_ONOFF,	0,	-1,	1, "Wisper Notice"};
ToggleVar 	tInputLine={					TOGGLEVAR_ONOFF,	1,	-1,	1, "Input line Patch"};
ToggleVar 	tUseCustomFont={				TOGGLEVAR_ONOFF,	0,	-1,	1, "Use Custom Font"};
static ConfigVar aConfigVars[] = {
	{CONFIG_VAR_TYPE_KEY, "WisperNoticeToggle",     &tWisperNotice      },
	{CONFIG_VAR_TYPE_KEY, "InputLineToggle",        &tInputLine         },
	{CONFIG_VAR_TYPE_KEY, "UseCustomFontToggle",    &tUseCustomFont     },
};
void GameChat_addConfigVars() {
	for (int i=0;i<_ARRAYSIZE(aConfigVars);i++) addConfigVar(&aConfigVars[i]);
}
BOOL fMyChatOn = FALSE;
void SendChatMessageW(wchar_t *myMsg)
{
	if( fInGame==FALSE )return;
	if( d2client_CheckUiStatus(UIVAR_CHATINPUT) ) return;

	fMyChatOn = TRUE;

	d2client_SetUiStatus( UIVAR_CHATINPUT , 0 , 1);
	wcscpy( d2client_pLastChatMessage, myMsg );
	D2MSG pmsg;
	pmsg.hwnd = d2gfx_GetHwnd();
	pmsg.wParam = 13;
	pmsg.xpos = 1;
	pmsg.ypos = 28;
	d2client_ChatInput(&pmsg);

	fMyChatOn = FALSE;
}
void SendChatMessage(char *myMsg) {
	if( fInGame==FALSE )return;
	if( d2client_CheckUiStatus(UIVAR_CHATINPUT) ) return;
	fMyChatOn = TRUE;
	d2client_SetUiStatus( UIVAR_CHATINPUT , 0 , 1);
	wchar_t *dst=d2client_pLastChatMessage;
	char *src=myMsg;
	do {*dst++=*src++;} while (*src);
	D2MSG pmsg;
	pmsg.hwnd = d2gfx_GetHwnd();
	pmsg.wParam = 13;
	pmsg.xpos = 1;
	pmsg.ypos = 28;
	d2client_ChatInput(&pmsg);
	fMyChatOn = FALSE;
}

void WisperPatch()
{
	if (tWisperNotice.isOn){
		static char NoticeMsg[] = "Someone wisper me.";
		SendChatMessage(NoticeMsg);
	}
}

void __declspec(naked) WisperPatch_ASM()
{
	__asm {
		pushad
		call WisperPatch;	
		popad
		mov ecx, 0x000014D4
		ret
	}

}

void CheckD2WinEditBox() {
	if (pD2WinEditBox) {
		if ( d2client_CheckUiStatus(UIVAR_CHATINPUT) ) {
			if( *(DWORD*)d2client_pLastChatMessage != 0xAD5AFFFF) {
				d2win_DestroyEditBox(pD2WinEditBox);
				pD2WinEditBox = NULL;
				FOCUSECONTROL = NULL;
			}
			d2win_SetTextFont(1);
		}
	}
}

BOOL __cdecl InputLinePatch(BYTE keycode)
{
	if ( fMyChatOn==FALSE && keycode != VK_ESCAPE) {
		if (keycode != VK_RETURN) return TRUE;
		if (pD2WinEditBox) {
			wcscpy(d2client_pLastChatMessage, d2win_GetEditBoxText(pD2WinEditBox));
		}
	}
	return FALSE;
}

void __declspec(naked) InputLinePatch1_ASM()
{
	__asm {
		mov ebx, eax
		push [edi+8]
		call InputLinePatch
		test eax, eax
		pop eax
		jz quitcode
		add dword ptr [esp], 0x427 // add to the point where function returns
quitcode:
		ret
	}
}

BOOL __stdcall EditBoxCallBack(D2EditBox* pEditBox, int dwLength, char *pChar)
{
	return TRUE;
}


BOOL __cdecl InitD2EditBox()
{	
	if ( fMyChatOn ) return FALSE;
	if(!pD2WinEditBox) {
		static DWORD dws[] = {0x0D, 0};//��������
		pD2WinEditBox = d2win_CreateEditBox(0x83,
			SCREENSIZE.y-58, SCREENSIZE.x-266,
			0x2D,
			0, 0, 0, 0, 0,
			sizeof(dws), &dws
			);
		FOCUSECONTROL = pD2WinEditBox;
		if (pD2WinEditBox) {
			d2win_SetEditBoxProc(pD2WinEditBox, &EditBoxCallBack);
			d2win_SetEditBoxText(pD2WinEditBox, d2client_pLastChatMessage);
		}
		*(DWORD*)d2client_pLastChatMessage = 0x0AD5AFFFF;
	}
	if (pD2WinEditBox)
		(pD2WinEditBox->fnCallBack)(pD2WinEditBox);
	return TRUE;
}

void __declspec(naked) InputLinePatch2_ASM()
{
	__asm {
		call InitD2EditBox
		test eax, eax
		jz quitcode
		add dword ptr [esp], 0x154
quitcode:
		mov ecx, d2client_pLastChatMessage
		ret
	}
}

BOOL __stdcall SelectTextPatch1(char*sDesc){
	
	if (pD2WinEditBox){
		DWORD dwStart;
		DWORD n;
		DWORD len;
		LPWSTR	lpText;
		lpText = (LPWSTR)(pD2WinEditBox->wszText);
		if (pD2WinEditBox->dwSelectStart == (DWORD)-1) return FALSE;
		if (pD2WinEditBox->dwSelectStart == pD2WinEditBox->dwSelectEnd) return FALSE;
		if (pD2WinEditBox->dwSelectStart > pD2WinEditBox->dwSelectEnd) {
			dwStart = pD2WinEditBox->dwSelectEnd;
			len = pD2WinEditBox->dwSelectStart - dwStart;
		} else {
			dwStart = pD2WinEditBox->dwSelectStart;
			len = pD2WinEditBox->dwSelectEnd - dwStart;
		}
		n = WideCharToMultiByte(CP_ACP, 0, lpText + dwStart, len, sDesc, (len+1)*2, NULL, NULL);
		sDesc[n] = 0;
		return TRUE;
	}
	return FALSE;
}

void __declspec(naked) SelectTextPatch1_ASM()
{
	__asm {
		push ecx
		push edx

		push edx
		call SelectTextPatch1
		test eax , eax

		pop edx
		pop ecx
		jz  org
		ret
org:
		mov byte ptr [esp+esi+0x88], 0
		ret;
	}
}


BOOL __stdcall SelectTextPatch2(char*sDesc){
	
	if (pD2WinEditBox){
		DWORD dwStart;
		DWORD n;
		LPWSTR	lpText;
		lpText = (LPWSTR)(pD2WinEditBox->wszText);
		if (pD2WinEditBox->dwSelectStart == (DWORD)-1) return FALSE;
		if (pD2WinEditBox->dwSelectStart == pD2WinEditBox->dwSelectEnd) return FALSE;
		if (pD2WinEditBox->dwSelectStart > pD2WinEditBox->dwSelectEnd) {
			dwStart = pD2WinEditBox->dwSelectEnd;
		} else {
			dwStart = pD2WinEditBox->dwSelectStart;
		}
		n = WideCharToMultiByte(CP_ACP, 0, lpText, dwStart, sDesc, (dwStart+1)*2, NULL, NULL);
		sDesc[n] = 0;
		return TRUE;
	}
	return FALSE;
}

void __declspec(naked) SelectTextPatch2_ASM()
{
	__asm {
		push ecx
		push edx

		push edx
		call SelectTextPatch2
		test eax , eax

		pop edx
		pop ecx
		jz  org
		ret
org:
		mov byte ptr [esp+ebp+0x88], 0
		ret;
	}
}