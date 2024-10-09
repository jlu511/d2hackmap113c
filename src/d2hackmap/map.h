enum AAR_Direction {AAR_BOTTOM=0,AAR_LEFT=1,AAR_RIGHT=2,AAR_TOP=3,};
struct MinimapLevelTarget {
	int id,dstLvl,ready,type;
	wchar_t *enName,*chName;
	int drawX,drawY,unitX,unitY,tileX,tileY,unitType,unitTxt;
	struct MinimapLevelTarget *next;
	struct RouteRect *dstRect,*rectRoute;
	struct RouteTile *dstTile,*tileRoute,*tiles;
	struct MinimapLevelTarget *recentNext;
	struct MinimapLevel *level;
};
struct MinimapLevel {
	int lvl,cur,n;
	struct MinimapLevelTarget *targets,*curTarget;
	int revealed;
	int tileX,tileY,tileW,tileH;
};
extern ToggleVar	tAutoMap;
extern BYTE anMonsterColours[1000];

int RevealCurAct();
int RevealLevelPlayerIn();
int NextLevelTarget();
int AutoTeleportGetTarget(POINT *dst,int *unitType,int *unitTxt);

void draw2map(POINT *minimap,int drawX,int drawY);
void tile2map(POINT* pos, int x, int y);