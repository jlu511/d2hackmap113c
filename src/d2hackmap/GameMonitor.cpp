﻿#include "stdafx.h"
#include "header.h"
#include "bitstream.h"
#include "multi.h"
#include "auto.h"

extern int dwCpuUser,dwCpuKernel,dwLoopFPS,dwDrawFPS;
int dwRecheckSelfItemMs=0;
UnitAny *leftWeapon=0,*rightWeapon=0;
int dwTownPortalCount=0,dwIdentifyPortalCount=0,fUsingBow=0,fUsingCrossBow=0,fUsingThrow=0;
int dwHPotionCount=0,dwMPotionCount=0,dwRPotionCount=0,dwArrowCount=0,dwCArrowCount=0,dwThrowCount=0;

void ShowGameCount();
ToggleVar 		tCountDown={	TOGGLEVAR_ONOFF,	0,	-1,	1,	"Count Down"};
ToggleVar 		tDrawRuneCollectorCount={	TOGGLEVAR_ONOFF,	0,	-1,	1,	"Draw Rune Collector Count"};
int 				dwGameMonitorX=			-10;
int 				dwGameMonitorY=			-110;
int 				dwCountDownFontSize=			3;
int 				dwCountDownGap=			25;
int 				dwCountDownFlashSecond=			30;
int 				fMonitorQuantity=			0;
int 				fMonitorDurability=			0;
ToggleVar 	tKillCount={				TOGGLEVAR_ONOFF,	0,	-1,		1, "KillCount",	};
ToggleVar 	tGetHitCount={			TOGGLEVAR_ONOFF,	0,	-1,		1, "GitHitCount",	};
int 		dwKillSum=				0;
int 		dwGameKills=			0;
int 		dwLastKills=			0;
int 		dwGetHitSum=			0;
int 		dwGetHits=				0;
int 		dwOrgMode=				0;
char targetNameColor=0;
static ConfigVar aConfigVars[] = {
	{CONFIG_VAR_TYPE_INT,"TargetNameColor",&targetNameColor,1},
	{CONFIG_VAR_TYPE_INT, "MonitorQuantity",     &fMonitorQuantity,  4},
	{CONFIG_VAR_TYPE_INT, "MonitorDurability",     &fMonitorDurability,  4},
	{CONFIG_VAR_TYPE_KEY, "CountDown",		&tCountDown},
	{CONFIG_VAR_TYPE_KEY, "DrawRuneCollectorCount",		&tDrawRuneCollectorCount},
	{CONFIG_VAR_TYPE_INT, "CountDownFontSize",		&dwCountDownFontSize,4},
	{CONFIG_VAR_TYPE_INT, "GameMonitorX",		&dwGameMonitorX,4},
	{CONFIG_VAR_TYPE_INT, "GameMonitorY",		&dwGameMonitorY,4},
	{CONFIG_VAR_TYPE_INT, "CountDownGap",		&dwCountDownGap,4},
	{CONFIG_VAR_TYPE_INT, "CountDownFlashSecond",		&dwCountDownFlashSecond,4},
	{CONFIG_VAR_TYPE_KEY,     "KillCountToggle",		&tKillCount         },
	{CONFIG_VAR_TYPE_KEY,     "GetHitCountToggle",	&tGetHitCount         },
};
void gamemonitor_addConfigVars() {
	for (int i=0;i<_ARRAYSIZE(aConfigVars);i++) addConfigVar(&aConfigVars[i]);
}
void gamemonitor_initConfigVars() {
	dwGameMonitorX=-10;
	dwGameMonitorY=-110;
	dwCountDownFontSize=3;
	dwCountDownGap=25;
	dwCountDownFlashSecond=30;
}

extern int dwNecInfoExpireMs;

int guessEnchantLevel();
int guessBOLevel(int debug);
void drawNpcTradeInfo(int color);

void hex(FILE *fp,int addr,void *buf,int n);
int fState100HP=0,fState106Mana=0;
int dwOakSageLvl;
static int fBC=0;
int dwGuessBOMs=0;
int dwGuessEnchantMs=0;
struct CountDown {
	const char *name;
	int skill;
	int addS,stepS,startMs,endMs;
	int active,lv,selfskill;
};
//werewolf: time 40s
//Lycanthropy: time+=20+lvl*20
int fWerewolf,fWerebear;
static struct CountDown cd_QH={"QH",52,120,24,0,0,0}; //sor fire enchant
static struct CountDown cd_BO={"BO",149,30,10,0,0,0}; //bar warcries battleorder
static struct CountDown cd_HS={"HS",117,5,25,0,0,0};//pal combat holyshield
static struct CountDown cd_werewolf={"Wolf",223,60,20,0,0,0};//dru shape werewolf
static struct CountDown cd_werebear={"Bear",228,60,20,0,0,0};//dru shape werebear
static struct CountDown cd_Fade={"Fade",267,108,12,0,0,0};//asn shadow fade
static struct CountDown cd_Burst={"Burst",258,108,12,0,0,0};//asn shadow burst of speed
static struct CountDown cd_Cloak={"Cloak",264,8,1,0,0,0};//asn shadow Cloak of Shadows
static struct CountDown cd_KD={"KD",0,0,0,0,0,0};//KD countdown
static struct CountDown cd_KB={"KB",0,0,0,0,0,0};//KB countdown
struct CountDown *countDowns[]={&cd_QH,&cd_BO,&cd_HS,
	&cd_werewolf,&cd_werebear,
	&cd_Fade,&cd_Burst,&cd_Cloak,&cd_KD,&cd_KB};

unsigned char beltCount[4],beltType[4];
int dwNeedPotionType; //bit 1:HP 2:Mana 3:Rejuvenation
int beltLayers;
int dwNextHPotionId,dwNextMPotionId;
static int minHPotionType,minMPotionType;
static int throwWeaponIdx;
static int hasRuneCollectorInv,hasRuneCollectorStash;
int dwThrowWeaponId,dwNextStackingId,dwNextStackX,dwNextStackY,dwNextStackLoc;

static void checkWeapon(UnitAny *pUnit) {
	if (pUnit->dwUnitType!=UNITNO_ITEM) return;
	ItemTxt *pItemTxt=d2common_GetItemTxt(pUnit->dwTxtFileNo);
	switch (pItemTxt->nType) {
		case 27:case 85:fUsingBow=1;break;
		case 35:fUsingCrossBow=1;break;
		case 38://Throwing Potion
		case 42:case 43://Throwing
		case 44:case 87://Javelins
			if (pUnit->pItemData->dwQuality!=ITEM_QUALITY_UNIQUE) {
				fUsingThrow=1;dwThrowWeaponId=pUnit->dwUnitId;throwWeaponIdx=GetItemIndex(pUnit->dwTxtFileNo)+1;break;
			}
	}
}
void usePotion(int mana) {
	int *id=mana?&dwNextMPotionId:&dwNextHPotionId;
	if (!(*id)) return;
	UnitAny *pUnit=d2client_GetUnitFromId(*id,UNITNO_ITEM);
	if (!pUnit) return;
	BYTE packet[13];
	if (pUnit->pItemData->nLocation==2) { //belt
		packet[0]=0x26;*(DWORD*)&packet[1]=*id;
		*(DWORD*)&packet[5]=0;*(DWORD*)&packet[9]=0;
	} else {
		packet[0]=0x20;*(DWORD*)&packet[1]=*id;
		*(DWORD*)&packet[5]=PLAYER->pMonPath->wUnitX;*(DWORD*)&packet[9]=PLAYER->pMonPath->wUnitY;
	}
	SendPacket(packet,13);
	*id=0;
}
//location: 0:bag 1:belt 2:cube
static void checkItem(UnitAny *pUnit,int location) {
	int type=0,lv=0;
	int index = GetItemIndex(pUnit->dwTxtFileNo)+1;
	switch (index) {
		case 2019:dwArrowCount+=d2common_GetUnitStat(pUnit, 70, 0);break;
		case 2021:dwCArrowCount+=d2common_GetUnitStat(pUnit, 70, 0);break;
		case 2008:case 2009:type=3;break;
		case 2080:case 2081:case 2082:case 2083:case 2084:type=1;lv=index-2080;break; //H1:45 H2:90 H3:100 H5:320
		case 2085:case 2086:case 2087:case 2088:case 2089:type=2;lv=index-2085;break; //M1:40 M2:80 M4:300 M5:500
	}
	lv<<=16;
	if (location==0) lv|=(pUnit->pItemPath->unitX<<8)|(pUnit->pItemPath->unitY<<4);
	if (index==throwWeaponIdx) {
		int left=d2common_GetUnitStat(pUnit, 70, 0);
		int usable=left-dwAutoThrowMinQuantity;
		if (usable>0) dwThrowCount+=usable;
		if (!dwNextStackingId&&left+d2common_GetUnitStat(PLAYER, 70, 0)>60) {
			dwNextStackingId=pUnit->dwUnitId;
			dwNextStackX=pUnit->pItemPath->unitX;
			dwNextStackY=pUnit->pItemPath->unitY;
			dwNextStackLoc=pUnit->pItemData->nItemLocation;
		}
	}
	if (location==2) return;
	if (location==1) {
		int x=pUnit->pItemPath->unitX;
		beltCount[x&3]++;
		if (0<=x&&x<=3) beltType[x&3]=type;
		lv|=(1<<15);
	}
	if (!type) return;
	switch (type) {
		case 1:if (lv<minHPotionType) {dwNextHPotionId=pUnit->dwUnitId;minHPotionType=lv;}
			dwHPotionCount++;break;
		case 2:if (lv<minMPotionType) {dwNextMPotionId=pUnit->dwUnitId;minMPotionType=lv;}
			dwMPotionCount++;break;
		case 3:dwRPotionCount++;break;
	}
}
void checkAutoSkill();
void recheckSelfItems() {
	static int n=0;
	if (!fInGame) return;
	int ub=fUsingBow,ucb=fUsingCrossBow,ut=fUsingThrow;
	*(int *)beltCount=0;*(int *)beltType=0;dwNeedPotionType=0;
	leftWeapon=NULL;rightWeapon=NULL;fUsingBow=0;fUsingCrossBow=0;fUsingThrow=0;
	beltLayers=1;dwNextHPotionId=0;dwNextMPotionId=0;dwHPotionCount=0;dwMPotionCount=0;dwRPotionCount=0;
	minHPotionType=0x7FFFFFFF;minMPotionType=0x7FFFFFFF;
	throwWeaponIdx=0;dwArrowCount=0;dwCArrowCount=0;dwThrowCount=0;dwNextStackingId=0;
	dwThrowWeaponId=0;
	for (UnitAny *pUnit = d2common_GetFirstItemInInv(PLAYER->pInventory);pUnit;pUnit = d2common_GetNextItemInInv(pUnit)) {
		if (pUnit->dwUnitType!=UNITNO_ITEM) continue;
		if (pUnit->pItemData->nLocation!=3) continue; //not on body
		if (pUnit->pItemData->nItemLocation==255) { //equipped
			switch (pUnit->pItemData->nBodyLocation) {
				case 4:rightWeapon=pUnit;checkWeapon(pUnit);break;
				case 5:leftWeapon=pUnit;checkWeapon(pUnit);break;
				case 8: //belt
					int index = GetItemIndex(pUnit->dwTxtFileNo)+1;
					if (index==1039||index==1040) beltLayers=2;
					else if (index==1041||index==1042) beltLayers=3;
					else beltLayers=4;
					break;
			}
		} 
	}
	hasRuneCollectorInv=0;hasRuneCollectorStash=0;
	for (UnitAny *pUnit = d2common_GetFirstItemInInv(PLAYER->pInventory);pUnit;pUnit = d2common_GetNextItemInInv(pUnit)) {
		if (pUnit->dwUnitType!=UNITNO_ITEM) continue ;
		switch (pUnit->pItemData->nLocation) { 
			case 0:break;//ground
			case 1: {//cube/stash/inv
				int index = GetItemIndex(pUnit->dwTxtFileNo)+1;
				switch (pUnit->pItemData->nItemLocation) {
					case 0: {//bag
						if (index==RUNE_COLLECTOR_ID) hasRuneCollectorInv++;
						checkItem(pUnit,0);
						break;
					}
					//case 3:checkItem(pUnit,2);break;//cube
					case 4: {//stash
						if (index==RUNE_COLLECTOR_ID) hasRuneCollectorStash++;
						break;
					}
				}
				break;
			}
			case 2:checkItem(pUnit,1);break; //belt
			case 3:break;//body
		}
	}
	for (int i=0;i<4;i++) if (beltCount[i]<beltLayers) {
		if (beltType[i]) dwNeedPotionType|=1<<(beltType[i]-1);
	}
	dwRecheckSelfItemMs=0;
	if (ub!=fUsingBow||ucb!=fUsingCrossBow||ut!=fUsingThrow) checkAutoSkill();
}
void setKDCountDown(int en) {
	cd_KD.active=en;
	cd_KD.endMs=dwCurMs+15000;
}
void setKBCountDown(int team,int en) {
	cd_KB.active=en;
	cd_KB.lv=team;
	cd_KB.endMs=dwCurMs+11750;
}
void setCountDownEndTime(struct CountDown *cd) {
	int time=cd->addS+cd->lv*cd->stepS;
	if (cd->skill==223||cd->skill==228) { //werewolf werebear
		int lv=getSkillLevel(PLAYER,224); //Lycanthropy
		if (lv==0) time=40;
		else time=cd->addS+lv*cd->stepS;
	}
	cd->endMs=cd->startMs+time*1000;
	if (tPacketHandler.isOn) {
		LOG("%s self=%d level=%d time=%d\n",cd->name,cd->selfskill,cd->lv,time);
	}
}
void setupCountdown(struct CountDown *cd) {
	cd->lv=getSkillLevel(PLAYER,cd->skill); 
	cd->startMs=dwCurMs;
	cd->selfskill=1;
	setCountDownEndTime(cd);
}
void onRightSkillMap(int x,int y) {
	switch (dwRightSkill) {
		case 52: setupCountdown(&cd_QH);break;
		case 117: setupCountdown(&cd_HS);break;
		case 149: setupCountdown(&cd_BO);break;
		case 223: setupCountdown(&cd_werewolf);break;
		case 228: setupCountdown(&cd_werebear);break;
		case 258: setupCountdown(&cd_Burst);break;
		case 264: setupCountdown(&cd_Cloak);break;
		case 267: setupCountdown(&cd_Fade);break;
	}
}
void onRightSkillUnit(int type,int id) {
	switch (dwRightSkill) {
		//case 52: setupCountdown(&cd_QH);break;
		case 117: setupCountdown(&cd_HS);break;
		case 149: setupCountdown(&cd_BO);break;
		case 223: setupCountdown(&cd_werewolf);break;
		case 228: setupCountdown(&cd_werebear);break;
		case 258: setupCountdown(&cd_Burst);break;
		case 264: setupCountdown(&cd_Cloak);break;
		case 267: setupCountdown(&cd_Fade);break;
	}
}
void ResetMonitor(){
	dwGuessBOMs=0;dwOakSageLvl=0;
	dwGuessEnchantMs=0;
	fBC=0;fWerewolf=0;fWerebear=0;
	leftWeapon=NULL;rightWeapon=NULL;
	for (int i=0;i<_ARRAYSIZE(countDowns);i++) countDowns[i]->active=0;
}
void GameMonitorNewGame() {
	dwGetHits = 0;dwGameKills = 0;dwLastKills = 0;
	ResetMonitor();
}
void GameMonitorEndGame() {
	ResetMonitor();
}
extern wchar_t wszNpcTradeInfo[256];

void ShowGameCount() {
	int xpos = SCREENSIZE.x/2 + 70;
	int ypos = SCREENSIZE.y - 50;
	DWORD dwOldFone = d2win_SetTextFont(1);
	if ( tGetHitCount.isOn ){	
		wchar_t wszTemp[512];
		DWORD dwColor = tGetHitCount.value32-1 ;
		wcscpy( wszTemp , L"GetHits:" );
		d2win_DrawText(wszTemp, xpos,  ypos, dwColor, 0);
		wsprintfW(wszTemp ,L"%d" , dwGetHits );
		d2win_DrawText(wszTemp, xpos+65,  ypos, dwColor, 0);
		wsprintfW(wszTemp ,L"Total: %d" , dwGetHitSum );
		d2win_DrawText(wszTemp, xpos+105,  ypos, dwColor , 0);
		ypos = ypos-15;
	}
	if ( tKillCount.isOn ){	
		wchar_t wszTemp[512];
		DWORD dwColor = tKillCount.value32-1 ;
		wcscpy( wszTemp , L"Kills:" );
		d2win_DrawText(wszTemp, xpos+20,  ypos, dwColor, 0);
		wsprintfW(wszTemp ,L"%d" , dwGameKills );
		d2win_DrawText(wszTemp, xpos+65,  ypos, dwColor, 0);
		wsprintfW(wszTemp ,L"Total: %d" , dwKillSum );
		d2win_DrawText(wszTemp, xpos+105,  ypos, dwColor, 0);
	}
	d2win_SetTextFont(dwOldFone);
}
void __fastcall SetKills( DWORD newKills ){
	if ( (int)newKills > (int)dwLastKills ) {
		dwKillSum+=newKills-dwLastKills;
		dwGameKills+=newKills-dwLastKills;
	}
	dwLastKills = newKills;
}
void __declspec(naked) KillCountPatch_ASM() {
	//死亡一次会减少一次杀敌数，所以可能变负数
	//记录下上次值，如果小于上次，认为死亡~
	__asm{
		push esi
		push eax
		movsx ecx, si
		call SetKills
		pop eax
		pop esi
		test esi,0x8000
		ret 
	}
}

void __declspec(naked) UnitModeChangePatch_ASM() {
	__asm{
		cmp edi , 4
		jne org
		mov edx, dword ptr [esi]
		cmp edx, 0
		jne org
		cmp [dwOrgMode] ,0
		je gocount
		mov [dwOrgMode] ,0
		jmp org
gocount:
		mov edx , dword ptr [esi+0x10] //单机会连着两次，但战网模式正常,记录下上一次调用的原值，并在全局循环中重置
		mov [dwOrgMode] , edx 
		add [dwGetHits] , 1
		add [dwGetHitSum] , 1
org:
		mov edx, dword ptr [esi+0xC4]
		ret 
	}
}
void drawRuneCollectorCount() {
	wchar_t wbuf[32];
	InventoryBIN* pInvs=*d2common_pInventoryTxt;if (!pInvs) return;
	InventoryBIN *pInv=pInvs+16+12;
	int nGridWidth=pInv->grid.w;
	int nGridHeight=pInv->grid.h;
	int dx=(SCREENSIZE.x-800)/2;
	int dy=(SCREENSIZE.y-600)/2;
	d2win_SetTextFont(8);
	for (UnitAny *pUnit = d2common_GetFirstItemInInv(PLAYER->pInventory);pUnit;pUnit = d2common_GetNextItemInInv(pUnit)) {
		if (pUnit->dwUnitType!=UNITNO_ITEM) continue;
		if (pUnit->pItemData->nItemLocation!=0&&pUnit->pItemData->nItemLocation!=4) continue;
		int index = GetItemIndex(pUnit->dwTxtFileNo)+1;
		if (index!=RUNE_COLLECTOR_ID) continue;
		if (!pUnit->pStatList) continue;
		StatList *plist=pUnit->pStatList;
		if (!(plist->dwListFlag&0x80000000)) continue;
		if (!plist->sFullStat.pStats) continue;
		Stat *stat=&plist->sFullStat;
		int n=stat->wStats;
		if (n>=511) continue;
		InventoryBIN *pInv=pInvs+16;
		if (pUnit->pItemData->nItemLocation==4) pInv+=12; //stash
		int left=pInv->grid.x0+3;
		int bottom=pInv->grid.y0+nGridHeight-1;
		StatEx *se=stat->pStats;
		for (int i=0;i<n;i++) {
			int id=se[i].wStatId;if (id!=359) continue;
			int count=se[i].dwStatValue;
			int txt=se[i].wParam&0xFFFF;
			int x=pUnit->pItemPath->unitX;
			int y=pUnit->pItemPath->unitY;
			int xpos = left+x*nGridWidth+dx;
			int ypos = bottom+y*nGridHeight+dy;
			int index = GetItemIndex(txt)+1;
			if (2103<=index&&index<=2135) {
				int r=index-2102;
				wsprintfW(wbuf,L"R%d",r);
				drawBgText(wbuf,xpos,ypos-14,0,0x10);
			} else if (2050<=index&&index<=2079) {
				static char *gemNames[6]={"紫","黄","蓝","绿","红","白"};
				int t=index-2050;
				wsprintfW(wbuf,L"%hs%d",gemNames[t/5],(t%5)+1);
				drawBgText(wbuf,xpos,ypos-14,0,0x10);
			} else if (2090<=index&&index<=2094) {
				int t=index-2090;
				wsprintfW(wbuf,L"骷%d",t+1);
				drawBgText(wbuf,xpos,ypos-14,0,0x10);
			}
			wsprintfW(wbuf,L"%d",count);
			drawBgText(wbuf,xpos,ypos,0,0x10);
			break;
		}
	}
}

extern wchar_t wcsLevelTargetName[128];
extern int dwTargetDistance,dwTargetDistanceMs;
extern struct InstalledPatch *networkPatch;
void DrawMonitorInfo(){
	int sw,sh;
	ShowGameCount();
	if (dwGuessBOMs&&dwCurMs>dwGuessBOMs) {
		dwGuessBOMs=0;
		struct CountDown *cd=&cd_BO;
		cd->selfskill=0;
		cd->lv=guessBOLevel(0);
		setCountDownEndTime(cd);
	}
	if (dwGuessEnchantMs&&dwCurMs>dwGuessEnchantMs) {
		dwGuessEnchantMs=0;
		int lv=guessEnchantLevel();
		if (lv) {
			struct CountDown *cd=&cd_QH;
			cd->selfskill=0;cd->lv=lv;
			setCountDownEndTime(cd);
		}
	}
	wchar_t wszTemp[512];
	if (tShowTestInfo.isOn) {
		int pos=wsprintfW(wszTemp, L"(%d,%d)",*d2client_pMouseX,*d2client_pMouseY);
		int drawX=d2client_GetScreenDrawX()+*d2client_pMouseX;
		int drawY=d2client_GetScreenDrawY()+*d2client_pMouseY;
		int unitX=((drawX>>1)+drawY)>>4;
		int unitY=(drawY-(drawX>>1))>>4;
		int dis=getDistanceM256(dwPlayerX-unitX,dwPlayerY-unitY);
		dis=(dis*2/3)>>8;
		pos+=wsprintfW(wszTemp+pos, L" %dyard",dis);
		if (0) 
		pos+=wsprintfW(wszTemp+pos, L" %d,%d,%c",
			d2common_getSkillStatus(PLAYER,PLAYER->pSkill->pLeftSkill),
			d2common_getSkillStatus(PLAYER,PLAYER->pSkill->pRightSkill),
			*d2client_pCurFrame<*d2client_pCastingDelayEndFrame?'d':'-');
		pos+=wsprintfW(wszTemp+pos, L" FPS%d l%d d%d cpu(%d%%,%d%%)", 
			FPS,dwLoopFPS,dwDrawFPS,dwCpuUser,dwCpuKernel);
		for (int i=1;i<=d2winLastId;i++) {
			if (i==dwGameWindowId) continue;
			D2Window *pwin=&d2wins[i];
			if (dwCurMs>pwin->cpuMs) continue;
			pos+=wsprintfW(wszTemp+pos, L" %d:%d%%",i,pwin->cpu); 
		}
		d2win_DrawText(wszTemp, 230 , SCREENSIZE.y-50, 0, 0);
	}
	int xpos = SCREENSIZE.x+dwGameMonitorX;
	int ypos = SCREENSIZE.y+dwGameMonitorY;
	if (*d2client_pUiNewSkillOn) ypos-=45;
	DWORD dwTimer = dwCurMs;
	DWORD dwOldFone = d2win_SetTextFont(8);
	if (wszNpcTradeInfo[0]) {
		static int bms,bcolor=0;
		if (dwCurMs>bms) {bcolor=bcolor==1?2:1;bms=(dwCurMs+256)&0xFFFFFF00;}
		d2win_SetTextFont(3);
		if (!d2client_CheckUiStatus(UIVAR_NPCTRADE)) wszNpcTradeInfo[0]=0;
		else {
			d2win_DrawText(wszNpcTradeInfo, 120 , SCREENSIZE.y-75, bcolor, 0);
			d2win_SetTextFont(8);
			drawNpcTradeInfo(bcolor);
		}
		d2win_SetTextFont(8);
	}
	if (networkPatch) {
		static int bms,bcolor=0;
		if (dwCurMs>bms) {bcolor=bcolor==1?2:1;bms=(dwCurMs+256)&0xFFFFFF00;}
		drawBgTextLeft(L"Network Packet Patch",xpos,ypos,bcolor,0x10);ypos-=15;
	}
	if (wcsLevelTargetName[0]) {
		int pos=0;
		pos+=wsprintfW(wszTemp+pos, L"%s", wcsLevelTargetName);
		if (dwCurMs<dwTargetDistanceMs) pos+=wsprintfW(wszTemp+pos, L": %d", dwTargetDistance*2/3);
		drawBgTextLeft(wszTemp,xpos,ypos,targetNameColor,0x10);ypos-=15;
	}
	for (int i=1;i<=d2winLastId;i++) {
		if (i==dwGameWindowId) continue;
		D2Window *pwin=&d2wins[i];
		if (pwin->autoSkillId) {
			wsprintfW(wszTemp, L"%d: %c->%d",i,pwin->autoLeft?'L':'R',pwin->autoSkillId); 
			int x=xpos-GetTextWidth(wszTemp);if (x<0) x=0;
			d2win_DrawText(wszTemp, x , ypos, 0, 0);
			ypos = ypos -15;
		}
	}
	if (tShowTestInfo.isOn) {
		UnitAny *pSelectedUnit = d2client_GetSelectedUnit();
		if (pSelectedUnit) {
			int pos=0,color=0;
			pos+=wsprintfW(wszTemp+pos, L"%d:%d txt%d",  
				pSelectedUnit->dwUnitType,pSelectedUnit->dwUnitId,pSelectedUnit->dwTxtFileNo);
			if (pSelectedUnit->dwUnitType==2) {
				ObjectTxt *pObjTxt=pSelectedUnit->pObjectData->pObjectTxt;
				//d2common_GetObjectTxt(pSelectedUnit->dwTxtFileNo);
				pos+=wsprintfW(wszTemp+pos, L" subclass=%d map=%d",pObjTxt->nSubClass,pObjTxt->nAutoMap);
			}
			pos+=wsprintfW(wszTemp+pos, L" mode=%d", pSelectedUnit->dwMode);
			float dis=getPlayerDistanceYard(pSelectedUnit);
			int dis10=(int)(dis*10+0.5);
			pos+=wsprintfW(wszTemp+pos, L" %d.%d yard", dis10/10,dis10%10);
			pos+=wsprintfW(wszTemp+pos, L" size=%d", d2common_getUnitSize(pSelectedUnit));
			if (pSelectedUnit->dwUnitType==1) {
				//if (d2common_IsUnitBlocked(PLAYER,pSelectedUnit,2)) pos+=wsprintfW(wszTemp+pos, L" notvisible");
				if (d2common_IsUnitBlocked(PLAYER,pSelectedUnit,4)) {color=1;pos+=wsprintfW(wszTemp+pos, L" unattackable");}
				MonsterTxt *pMonTxt= pSelectedUnit->pMonsterData->pMonsterTxt;
				if (pMonTxt->fBoss)
					pos+=wsprintfW(wszTemp+pos, L" TB");
				if (pSelectedUnit->pMonsterData->fBoss) 
					pos+=wsprintfW(wszTemp+pos, L" B");
				if (pSelectedUnit->pMonsterData->fUnique) 
					pos+=wsprintfW(wszTemp+pos, L" U");
				if (pSelectedUnit->pMonsterData->fChamp)
					pos+=wsprintfW(wszTemp+pos, L" C");
			}

			int x=xpos-GetTextWidth(wszTemp);if (x<0) x=0;
			d2win_DrawText(wszTemp, x , ypos, color, 0);
			ypos = ypos -15;
		}
	}
	if (fMonitorQuantity) {
		int qx=(SCREENSIZE.x>>1)+150;
		int qy=SCREENSIZE.y-45;
		d2win_SetTextFont(8);
		if (dwHPotionCount) {
			wsprintfW(wszTemp,L"%d",dwHPotionCount);d2win_GetTextAreaSize(wszTemp, &sw, &sh);
			d2gfx_DrawRectangle(qx,qy-12,qx+sw,qy,0x0A,5);d2win_DrawText(wszTemp,qx,qy,0,0);qx+=sw+3;
		}
		if (dwMPotionCount) {
			wsprintfW(wszTemp,L"%d",dwMPotionCount);d2win_GetTextAreaSize(wszTemp, &sw, &sh);
			d2gfx_DrawRectangle(qx,qy-12,qx+sw,qy,0x91,5);d2win_DrawText(wszTemp,qx,qy,0,0);qx+=sw+3;
		}
		if (dwRPotionCount) {
			wsprintfW(wszTemp,L"%d",dwRPotionCount);d2win_GetTextAreaSize(wszTemp, &sw, &sh);
			d2gfx_DrawRectangle(qx,qy-12,qx+sw,qy,0x4B,5);d2win_DrawText(wszTemp,qx,qy,0,0);qx+=sw+3;
		}
		if (fUsingBow||fUsingCrossBow||fUsingThrow||dwRightSkill==35) {
			int pos=0,n=d2common_GetUnitStat(PLAYER, 70, 0); //quantity
			if (n) {
				d2win_SetTextFont(3);
				pos+=wsprintfW(wszTemp+pos, L"%d",  n);
				n=0;
				if (fUsingBow) n=dwArrowCount;
				else if (fUsingCrossBow) n=dwCArrowCount;
				else if (fUsingThrow) n=dwThrowCount;
				if (n) {
					pos+=wsprintfW(wszTemp+pos, L"+%d",  n);
				}
				d2win_GetTextAreaSize(wszTemp, &sw, &sh);
				int x=xpos-sw;
				d2gfx_DrawRectangle(x,ypos-12,x+sw,ypos,0x10,5);d2win_DrawText(wszTemp, x , ypos, 0, 0);
				ypos = ypos-25;
				d2win_SetTextFont(8);
			}
		}
		if (!d2client_CheckUiStatus(UIVAR_BELT)) {
			static wchar_t *num[4]={L"0",L"1",L"2",L"3"};
			int x=SCREENSIZE.x/2+33;
			int y=SCREENSIZE.y-40;
			for (int i=0;i<4;i++) {
				if (beltCount[i]<beltLayers) d2win_DrawText(num[beltCount[i]&7],x,y,0,0);
				x+=30;
			}
		}
		/*if (!fPlayerInTown) {
			wsprintfW(wszTemp, L"ID%d Town%d", dwIdentifyPortalCount,dwTownPortalCount);
			int x=xpos-GetTextWidth(wszTemp);
			d2win_DrawText(wszTemp, x , ypos, 0, 0);
			d2win_DrawText(wszTemp, x+1 , ypos+1, 6, 0);
			ypos = ypos -15;
		}
		*/
	}
	if (fMonitorDurability) {
		d2win_SetTextFont(8);
		for (int i=0;i<2;i++) {
			UnitAny *pUnit=i==0?rightWeapon:leftWeapon;
			if (!pUnit) continue;
			int max_durability=d2common_GetUnitStat(pUnit, 73, 0); 
			if (max_durability) {
				int durability=d2common_GetUnitStat(pUnit, 72, 0); 
				if (durability<max_durability) {
					wsprintfW(wszTemp, L"%s %d/%d",i==0?L"DurR":L"DurL",durability,max_durability);
					d2win_GetTextAreaSize(wszTemp, &sw, &sh);
					int x=xpos-sw;
					d2gfx_DrawRectangle(x,ypos-12,x+sw,ypos,0x10,5);d2win_DrawText(wszTemp, x , ypos, 0, 0);
					ypos = ypos -15;
				}
			}
		}
	}
	if (tCountDown.isOn) {
		d2win_SetTextFont(dwCountDownFontSize);
		for (int i=0;i<_ARRAYSIZE(countDowns);i++) {
			struct CountDown *cd=countDowns[i];
			if (!cd->active) continue;
			int color=6;
			int secs;
			if (cd->endMs) {
				int ms=cd->endMs-dwCurMs;
				secs = ms/1000;
				if (0<=secs&&secs<30) color=(ms%1000)<500?1:2;
				wsprintfW(wszTemp, L"%hs%d: %d", cd->name,cd->lv,secs);
			} else {
				secs=(dwCurMs-cd->startMs)/1000;
				wsprintfW(wszTemp, L"%hs: %d", cd->name,secs);
			}
			d2win_GetTextAreaSize(wszTemp, &sw, &sh);
			int x=xpos-sw;
			d2gfx_DrawRectangle(x,ypos-12,x+sw,ypos,0x10,5);d2win_DrawText(wszTemp, x , ypos, 0, 0);
			ypos = ypos-dwCountDownGap;
		}
		if (dwOakSageLvl) {
			wsprintfW(wszTemp, L"OakSage%d", dwOakSageLvl);
			d2win_GetTextAreaSize(wszTemp, &sw, &sh);
			int x=xpos-sw;
			d2gfx_DrawRectangle(x,ypos-12,x+sw,ypos,0x10,5);d2win_DrawText(wszTemp, x , ypos, 0, 0);
			ypos = ypos-dwCountDownGap;
		}
	}
	if (tDrawRuneCollectorCount.isOn) {
		if (hasRuneCollectorStash&&*d2client_pUiStashOn||hasRuneCollectorInv&&*d2client_pUiInventoryOn) {
			drawRuneCollectorCount();
		}
	}
	d2win_SetTextFont(dwOldFone);
}

void __stdcall SetState( DWORD dwStateNo , BOOL fSet ){
	//if (tPacketHandler.isOn) {LOG("set state %d %d\n",dwStateNo,fSet);}
	struct CountDown *cd=NULL;
	switch (dwStateNo) {
		case 16: cd=&cd_QH;break;
		case 23: if (!fSet&&fAutoSkill&&dwRightSkill==71) dwAutoSkillCheckMs=0;break; //Dim Vision
		case 32: cd=&cd_BO;break;
		case 51: 
			if (fBC!=fSet) {fBC=fSet;dwSkillChangeCount++;}
			break; //Battle Command
		case 58: if (!fSet&&fAutoSkill&&dwRightSkill==82) dwAutoSkillCheckMs=0;break; //Life Tap
		case 60: if (!fSet&&fAutoSkill&&dwRightSkill==87) dwAutoSkillCheckMs=0;break; //Decrepify
		case 100: fState100HP=fSet;break;
		case 101: cd=&cd_HS;break;
		case 106: fState106Mana=fSet;break;
		case 134: dwSkillChangeCount++;break; //ShrineSkill
		case 139: fWerewolf=fSet;cd=&cd_werewolf;break;
		case 140: fWerebear=fSet;cd=&cd_werebear;break;
		case 149: dwGuessBOMs=dwCurMs+500;break; //Oak Sage
		case 153: cd=&cd_Cloak;break;
		case 157: cd=&cd_Burst;break;
		case 159: cd=&cd_Fade;break;
	}
	if (cd) {
		cd->active=fSet;
		if (fSet) {
			if (cd->startMs<=dwCurMs&&dwCurMs<cd->startMs+500) {//self skill
				if (dwStateNo==16) dwGuessEnchantMs=dwCurMs+500;
				else if (dwStateNo==32) dwGuessBOMs=dwCurMs+500;
			} else {
				cd->endMs=0;
				if (dwStateNo==16) dwGuessEnchantMs=dwCurMs+500;
				else if (dwStateNo==32) dwGuessBOMs=dwCurMs+500;
			}
			cd->startMs=dwCurMs;
		} else {
			cd->startMs=cd->endMs=0;
		}
	}
}
static void __fastcall recvState(BYTE *buf) {
	int type=buf[1];
	int id=*(int *)(buf+2);
	int state=buf[0]==0xa8?buf[7]:buf[6];
	switch (buf[0]) {
		case 0xa7:LOG("Delayed State %d:%d %d\n",type,id,state);break;
		case 0xa8:LOG("State %d:%d %d\n",type,id,state);break;
		case 0xa9:LOG("End State %d:%d %d\n",type,id,state);break;
	}
}
void __fastcall autoSkillDimVision(char *packet);
//delayed state
void __declspec(naked) RecvCommand_A7_Patch_ASM() {
	__asm{
		cmp tPacketHandler.isOn,0
		je next
		push ecx
		call recvState
		pop ecx
next:
		mov esi, ecx
		movzx eax, byte ptr [esi+06] //stateId
		cmp eax,0x10
		jne not_enchant
		mov dwAutoSummonCheckEnchantMs,1
not_enchant:
		cmp eax,23
		jne not_dimvision
		push esi
		call autoSkillDimVision
		pop esi
not_dimvision:
		movzx edx, byte ptr [esi+1] //type
		mov ecx, dword ptr [esi+2] //id

		cmp edx , 0
		jne org
		cmp ecx ,[dwPlayerId]
		jne org
		
		push ecx
		push edx
		push esi 

		push 1
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
org:
		ret
	}
}

void __declspec(naked) RecvCommand_A8_Patch_ASM() {
	__asm{
		cmp tPacketHandler.isOn,0
		je next
		pushad
		call recvState
		popad
next:
		mov esi, ecx
		movzx eax, byte ptr [esi+07]
		cmp eax,0x10
		jne not_enchant
		mov dwAutoSummonCheckEnchantMs,1
not_enchant:
		movzx edx, byte ptr [esi+1]
		mov ecx, dword ptr [esi+2]
		cmp edx , 0
		jne  org
		cmp ecx ,[dwPlayerId]
		jne org
		
		push ecx
		push edx
		push esi 

		push 1
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
org:
		ret
	}
}

void __declspec(naked) RecvCommand_A9_Patch_ASM() {
	__asm{
		cmp tPacketHandler.isOn,0
		je next
		pushad
		call recvState
		popad
next:
		mov esi, ecx
		movzx eax, byte ptr [esi+6]
		cmp eax,0x10
		jne not_enchant
		mov dwAutoSummonCheckEnchantMs,1
not_enchant:
		movzx edx, byte ptr [esi+1]
		mov ecx, dword ptr [esi+2]

		cmp edx , 0
		jne  org
		cmp ecx ,[dwPlayerId]
		jne org
		
		push ecx
		push edx
		push esi 

		push 0
		push eax
		call SetState

		pop esi
		pop edx
		pop ecx
org:
		ret
	}
}

void itemAction(struct bitstream *bs,char *buf) {
	int cmd=buf[0]&0xFF;
	bitstream_seek(bs,61-16);
	int equipped=bitstream_rbit(bs, 4);
	bitstream_seek(bs,73-16);
	int location=bitstream_rbit(bs, 3);//bit73-75 0:belt bag:1 cube:4 stash:5
	if (EXPANSION) {
		dwSkillChangeCount++;
	} else {
		if (equipped) dwSkillChangeCount++;
	}
	if (location==0 //belt changed
		||equipped==4||equipped==5||equipped==11||equipped==12 //weapon changed
		) {
		leftWeapon=NULL;rightWeapon=NULL;
	}
	dwRecheckSelfItemMs=dwCurMs+300;
	if (tPacketHandler.isOn&&logfp) {
		LOG("	RECV %02x_itemAction location=%d equipped=%d\n",buf[0]&0xFF,location,equipped);
		if ((buf[0]&0xFF)==0x9C) hex(logfp,0,buf,8);
		else hex(logfp,0,buf,13);
/*|  1    | Helmet                  |
|  2    | Amulet                  |
|  3    | Armor                   |
|  4    | Weapon (Right)          |
|  5    | Weapon (Left)           |
|  6    | Ring (Right)            |
|  7    | Ring (Left)             |
|  8    | Belt                    |
|  9    | Boots                   |
| 10    | Gloves                  |
| 11    | Alternate Weapon (Right)|
| 12    | Alternate Weapon (Left) |
*/
	}
}
void __fastcall itemAction9C(char *buf) {
	struct bitstream bs;
	bitstream_init(&bs,buf+8,buf+8+12);
	itemAction(&bs,buf);
}
void __fastcall itemAction9D(char *buf) {
	struct bitstream bs;
	bitstream_init(&bs,buf+13,buf+13+12);
	itemAction(&bs,buf);
}

void __fastcall updateItemSkill(char *buf) {
	//22 [WORD Unknown (Unit Type?)] [DWORD Unit Id] [WORD Skill] [BYTE Amount] [WORD Unknown] 
	//22 00 db 01 - 00 00 00 dc - 00 02 a9 00 -             |"               
	int skill=*(unsigned short *)(buf+7);
	int count=buf[9];
	if (skill==220) dwTownPortalCount=count;
	else if (skill==218) dwIdentifyPortalCount=count;
	if (tPacketHandler.isOn) 
		LOG("	RECV 22_updateItemSkill skill=%d count=%d\n", skill,count);
}

// 6FB5E090 - B9 3F000000           - mov ecx,0000003F { 63 }
void __declspec(naked) RecvCommand_9C_Patch_ASM() {
	__asm{
		pushad
		call itemAction9C
		popad
		mov ecx, 0x3F
		ret
	}
}
// 6FB5E880 - B9 40000000           - mov ecx,00000040 { 64 }
void __declspec(naked) RecvCommand_9D_Patch_ASM() {
	__asm{
		pushad
		call itemAction9D
		popad
		mov ecx, 0x40
		ret
	}
}
/*
6FB5C800 - 56                    - push esi
6FB5C801 - 8B F1                 - mov esi,ecx
6FB5C803 - 8A 46 0B              - mov al,[esi+0B]
6FB5C806 - 84 C0                 - test al,al
*/
void __declspec(naked) RecvCommand_22_Patch_ASM() {
	__asm{
		pushad
		call updateItemSkill
		popad
		mov esi,ecx
		mov al,byte ptr [esi+0xB]
		ret
	}
}
