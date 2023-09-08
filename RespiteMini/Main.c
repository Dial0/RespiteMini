#define tBASE           0b0000000000000001
#define tSWAMP          0b0000000000000010
#define tDESERT         0b0000000000000100
#define tBARREN         0b0000000000001000
#define tSWAMP_WATER    0b0000000000010000
#define tLIGHT_GRASS    0b0000000000100000
#define tDARK_GRASS     0b0000000001000000
#define tSNOW           0b0000000010000000
#define tTUNDRA         0b0000000100000000
#define tCLIFFS_MOUNT   0b0000001000000000
#define tWATER          0b0000010000000000
#define tTREES          0b0000100000000000
#define tBRIDGE_HOUSE   0b0001000000000000

//#define sprintf_s(buf, ...) snprintf((buf), sizeof(buf), __VA_ARGS__)

#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

enum MoveDir { North, South, East, West };

typedef struct RenderParams {
    int mapViewPortWidth;
    int mapViewPortHeight;
    int mapWidth;
    int mapHeight;
    int tileSize;
    int smoothScrollY;
    int scale;
} RenderParams;

typedef struct iVec2 {
    int x;
    int y;
} iVec2;

typedef struct WagonEntity {
    iVec2 wagonTilePos;
    Vector2 wagonWorldPos;
    iVec2 wagonTargetTilePos;
    int aniFrames;
    float moveSpeed;
    enum MoveDir aniDir;
} WagonEntity;

typedef struct WagonAni {
    Texture2D Tex;

    //FrameRecs
    Rectangle North;
    Rectangle altNorth;

    Rectangle East;
    Rectangle altEast;

    Rectangle West;
    Rectangle altWest;

    Rectangle South;
    Rectangle altSouth;

} WagonAni;

Font font;
int fontSize = 15;

typedef struct State {
    int tileSize;
    int baseSizeX;
    int baseSizeY;
    int scale ;
    int screenWidth;
    int screenHeight;

    Texture2D map;
    Texture2D ui;
    Texture2D wagon;

    WagonAni wagonAni;

    int smoothScrollY;
    iVec2 cursTilePos;
    RenderTexture2D mapRendTex;
    RenderTexture2D uiRendTex;
    RenderParams renderParams;

    int movingWagon;
    WagonEntity WagonEnt;
    int path[12];
    int pathsize;
    int movePathIdx;
    int totalPathCost;

    int mapDataSize;
    unsigned char* mapData;
    int mapSizeX;
    int mapSizeY;
}State;

typedef struct TileResources {
    char water;
    char wood;
    char animals;
    char fish;
    char plants;
    char minerals;
    char soil;
    char town;
} TileResources;
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

//Returns the screenXY of the top left pixel of the tile
iVec2 mapTileXYtoScreenXY(int MapX, int MapY, RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* param.tileSize, (-MapY + tileYOffset)* param.tileSize + mapRenderOffset % param.tileSize + scrollOffset };
}

iVec2 mapWorldXYtoScreenXY(float MapX, float MapY, RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* (float)(param.tileSize), (-MapY + tileYOffset)* (float)(param.tileSize) + mapRenderOffset % param.tileSize + scrollOffset };
}

void drawDebugGrid(RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int scrollOffset = (param.smoothScrollY / param.scale);
    int tileSize = param.tileSize;
    int gridoffset = (mapRenderOffset + scrollOffset) % (tileSize * 2);

    for (size_t y = 0; y < param.mapViewPortHeight / tileSize + 3; y++) {
        for (size_t x = 0; x < param.mapWidth / tileSize; x++) {
            if ((x + y) % 2)
            {
                DrawRectangleLines(x * tileSize, y * tileSize + gridoffset, tileSize, tileSize, BLUE);
            }
            else
            {
                DrawRectangleLines(x * tileSize, y * tileSize + gridoffset, tileSize, tileSize, RED);
            }
        }
    }
}

int moveWagon(WagonEntity* wagon) {

    //if we are already at the target return
    if (wagon->wagonTilePos.x == wagon->wagonTargetTilePos.x
        && wagon->wagonTilePos.y == wagon->wagonTargetTilePos.y) {
        return 1;
    }
    Vector2 target = { wagon->wagonTargetTilePos.x, wagon->wagonTargetTilePos.y };
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, wagon->wagonWorldPos));

    //update wagons directions for render pass

    if (fabs(dir.x) > fabs(dir.y)) { // moving sideways
        if (dir.x < 0) { // moving West
            wagon->aniDir = West;
        }
        else { //moving East
            wagon->aniDir = East;
        }
    }
    else { // moving up/down
        if (dir.y < 0) { // moving North
            wagon->aniDir = South;
        }
        else { //moving South
            wagon->aniDir = North;
        }
    }

    wagon->wagonWorldPos = Vector2Add(Vector2Scale(dir, wagon->moveSpeed), wagon->wagonWorldPos);

    wagon->aniFrames += 1;

    if (Vector2Distance(wagon->wagonWorldPos, target) <= wagon->moveSpeed){
        wagon->wagonTilePos = wagon->wagonTargetTilePos;
        wagon->wagonWorldPos.x = wagon->wagonTilePos.x;
        wagon->wagonWorldPos.y = wagon->wagonTilePos.y;
        wagon->aniFrames = 0;
        return 1;
    }

    return 0;

}

void renderWagon(WagonEntity wagonEnt, RenderParams renderParams, WagonAni Ani ) {
    iVec2 newWagPixel = mapWorldXYtoScreenXY(wagonEnt.wagonWorldPos.x, wagonEnt.wagonWorldPos.y, renderParams);

    bool alternateAnimation = ((int)wagonEnt.aniFrames / 10) % 2;

    if (wagonEnt.aniDir == North) {
        
        if (alternateAnimation) {
            DrawTextureRec(Ani.Tex, Ani.altNorth, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
        else {
            DrawTextureRec(Ani.Tex, Ani.North, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
    }

    if (wagonEnt.aniDir == East) {
        if (alternateAnimation) {
            DrawTextureRec(Ani.Tex, Ani.altEast, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
        else {
            DrawTextureRec(Ani.Tex, Ani.East, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
    }

    if (wagonEnt.aniDir == West) {
        if (alternateAnimation) {
            DrawTextureRec(Ani.Tex, Ani.altWest, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
        else {
            DrawTextureRec(Ani.Tex, Ani.West, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
    }

    if (wagonEnt.aniDir == South) {
        if (alternateAnimation) {
            DrawTextureRec(Ani.Tex, Ani.altSouth, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
        else {
            DrawTextureRec(Ani.Tex, Ani.South, (struct Vector2) { newWagPixel.x, newWagPixel.y }, WHITE);
        }
    }




}

int calcTileMoveCost(unsigned int tileData) {

    int moveCost = 0;
    int terrains = 0;

    if (tSWAMP & tileData) {
        moveCost += 3;
        terrains += 1;
    }
    if (tDESERT & tileData) {
        moveCost += 4;
        terrains += 1;
    }
    if (tBARREN & tileData) {
        moveCost += 2;
        terrains += 1;
    }
    if (tLIGHT_GRASS & tileData) {
        moveCost += 1;
        terrains += 1;
    }
    if (tDARK_GRASS & tileData) {
        moveCost += 2;
        terrains += 1;
    }
    if (tSNOW & tileData) {
        moveCost += 3;
        terrains += 1;
    }
    if (tTUNDRA & tileData) {
        moveCost += 4;
        terrains += 1;
    }

    moveCost /= terrains; //terrain movement cost is averaged over the terrains that occupy that tile

    if (tSWAMP_WATER & tileData) {
        moveCost += 2;
    }
    if (tCLIFFS_MOUNT & tileData) {
        moveCost += 3;
    }
    if (tWATER & tileData) {
        moveCost += 1;
    }
    if (tTREES & tileData) {
        moveCost += 1;
    }



    if (tBRIDGE_HOUSE & tileData) {
        moveCost = 1; //towns and bridges are always the lowest move cost
    }

    return moveCost;
}

void renderUiWindow(Texture2D ui, int windowX, int windowY, int windowSizeX, int windowSizeY) {
    int cornerTileSize = 16;

    Rectangle topLeftSrc = { 128,0,16,16 };
    Rectangle topRightSrc = { 160,0,16,16 };
    Rectangle bottomLeftSrc = { 128,32,16,16 };
    Rectangle bottomRightSrc = { 160,32,16,16 };

    Vector2 topLeft = { windowX ,windowY};
    Vector2 topRight = { windowX + windowSizeX - cornerTileSize,windowY };
    Vector2 bottomLeft = { windowX, windowY + windowSizeY - cornerTileSize };
    Vector2 bottomRight = { windowX + windowSizeX - cornerTileSize, windowY + windowSizeY - cornerTileSize };

    Rectangle topBarSrc = { 144,0,16,16 };
    Rectangle bottomBarSrc = { 144,32,16,16 };
    Rectangle leftBarSrc = { 128,16,16,16 };
    Rectangle rightBarSrc = { 160,16,16,16 };

    Rectangle topBar = { windowX + cornerTileSize,windowY, windowSizeX - (cornerTileSize * 2),cornerTileSize };
    Rectangle bottomBar = { windowX + cornerTileSize,windowY + windowSizeY - cornerTileSize, windowSizeX - (cornerTileSize * 2),cornerTileSize };
    Rectangle leftBar = { windowX,windowY + cornerTileSize, cornerTileSize,windowSizeY - (cornerTileSize * 2) };
    Rectangle rightBar = { windowX + windowSizeX - cornerTileSize,windowY + cornerTileSize,cornerTileSize ,windowSizeY - (cornerTileSize * 2) };

    Rectangle centerSrc = { 144,16,16,16 };

    Rectangle center = { windowX + cornerTileSize,windowY + cornerTileSize, windowSizeX - (cornerTileSize * 2),windowSizeY- (cornerTileSize * 2) };

    DrawTextureRec(ui, topLeftSrc, topLeft, WHITE);
    DrawTextureRec(ui, topRightSrc, topRight, WHITE);
    DrawTextureRec(ui, bottomLeftSrc, bottomLeft, WHITE);
    DrawTextureRec(ui, bottomRightSrc, bottomRight, WHITE);

    DrawTexturePro(ui, topBarSrc, topBar, (struct Vector2) { 0, 0 }, 0.0f, WHITE);
    DrawTexturePro(ui, bottomBarSrc, bottomBar, (struct Vector2) { 0, 0 }, 0.0f, WHITE);
    DrawTexturePro(ui, leftBarSrc, leftBar, (struct Vector2) { 0, 0 }, 0.0f, WHITE);
    DrawTexturePro(ui, rightBarSrc, rightBar, (struct Vector2) { 0, 0 }, 0.0f, WHITE);

    DrawTexturePro(ui, centerSrc, center, (struct Vector2) { 0, 0 }, 0.0f, WHITE);
}

void renderTileInfoDebug(int x, int y, unsigned int tileData) {
    int textYHeight = y;
    int textHeight = 32;
    int textSpacing = 4;

    int tileMoveCost = calcTileMoveCost(tileData);

    char str[80];

    //sprintf_s(str,80, "Move Cost: %i", tileMoveCost);
    
    //DrawText(str, x, textYHeight, textHeight, RED);
    textYHeight += textHeight + textSpacing;

    if (tSWAMP & tileData) {
        DrawText("Swamp", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tDESERT & tileData) {
        DrawText("Desert", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tBARREN & tileData) {
        DrawText("Barrens", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tSWAMP_WATER & tileData) {
        DrawText("Deep Swamp", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tLIGHT_GRASS & tileData) {
        DrawText("Medow", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tDARK_GRASS & tileData) {
        DrawText("Grass", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tSNOW & tileData) {
        DrawText("Snow", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tTUNDRA & tileData) {
        DrawText("Tundra", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tCLIFFS_MOUNT & tileData) {
        DrawText("Mountain", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tWATER & tileData) {
        DrawText("Water", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tTREES & tileData) {
        DrawText("Trees", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
    if (tBRIDGE_HOUSE & tileData) {
        DrawText("Town", x, textYHeight, textHeight, RED);
        textYHeight += textHeight + textSpacing;
    }
}

void renderTileInfo(Texture2D ui,int posX, int posY, int tileId, int level) {
    Rectangle iconSrc = { tileId*32,64,32,32 };
    DrawTextureRec(ui, iconSrc,(struct Vector2) { posX, posY}, WHITE);

    char* tileName;

    if (tileId == 0) {
        tileName = "Swamp";
    } else if (tileId == 1) {
        tileName = "Desert";
    } else if (tileId == 2) {
        tileName = "Barrens";
    } else if (tileId == 3) {
        tileName = "Tussock";
    } else if (tileId == 4) {
        tileName = "Grasslands";
    } else if (tileId == 5) {
        tileName = "Snow";
    } else if (tileId == 6) {
        tileName = "Tundra";
    } else {
        tileName = "Forrest";
    }
    
    DrawTextEx(font, tileName, (struct Vector2) { posX + 32 + 2, posY }, fontSize, 1, (Color){218,165,84,255});

    if (level == 3) {
        DrawTextEx(font, "Prominent", (struct Vector2) { posX + 32 + 2, posY + fontSize + 2 }, fontSize, 1, (Color){218,165,84,255});
    } else if (level == 2){
        DrawTextEx(font, "Moderate", (struct Vector2) { posX + 32 + 2, posY + fontSize + 2 }, fontSize, 1, (Color){218,165,84,255});
    } else {
        DrawTextEx(font, "Sparse", (struct Vector2) { posX + 32 + 2, posY + fontSize + 2 }, fontSize, 1, (Color){218,165,84,255});
    }
}

void renderTileInfoWindow(Texture2D ui, int windowX, int windowY, int windowSizeX, int windowSizeY, unsigned int tileData) {
    int edgeSpacing = 8;

    renderUiWindow(ui,windowX,windowY,windowSizeX,windowSizeY);
    
    int tileInfoY = windowY + edgeSpacing + 32;
    int tileInfoSpacing = 36;

    int terrainFlags = 0b0000000111101110;

    int tileTerrains = tileData & terrainFlags;

    int numTerrains = 0;
    for (int i = 0; i < 32; i++) {
        if((tileTerrains >> i) & 0b0000000000000001) numTerrains += 1;
    }

    int terrainLevel = 0;

    if(numTerrains >= 3) {
        terrainLevel = 1;
    } else if (numTerrains == 2) {
        terrainLevel = 2;
    } else {
        terrainLevel = 3;
    }

    if (tSWAMP & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,0,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }

    if (tDESERT & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,1,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tBARREN & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,2,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tSWAMP_WATER & tileData) {

    }
    if (tLIGHT_GRASS & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,3,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tDARK_GRASS & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,4,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tSNOW & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,5,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tTUNDRA & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,6,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tCLIFFS_MOUNT & tileData) {

    }
    if (tWATER & tileData) {

    }
    if (tTREES & tileData) {
        renderTileInfo(ui,windowX + edgeSpacing,tileInfoY,7,terrainLevel);
        tileInfoY += tileInfoSpacing;
    }
    if (tBRIDGE_HOUSE & tileData) {

    }
}

unsigned int getTileData(unsigned char x, unsigned char y, unsigned char* mapData) {
    unsigned char width = mapData[0];
    unsigned char height = mapData[1];
    int tileIdx = y * width + x;
    unsigned int dataPos = 2 + tileIdx * 4;
    unsigned int tileData;
    memcpy(&tileData, mapData + dataPos, 4);
    return tileData;
}

iVec2 mapIdxToXY(int tileIdx, int mapSizeX) {
    int x = tileIdx % mapSizeX;
    int y = tileIdx / mapSizeX;
    return (struct iVec2) { x, y };
}

int hDistNodes(int start, int end, int mapSizeX) {
    iVec2 startPos = mapIdxToXY(start, mapSizeX);
    iVec2 endPos = mapIdxToXY(end, mapSizeX);
    return abs(endPos.x - startPos.x) + abs(endPos.y - startPos.y);
}

int getNeighbour(int tileIdx, int dir, int mapSizeX, int mapSizeY) {
    int i = tileIdx;
    if (dir == 0) {
        //North
        if (i + mapSizeX < (mapSizeX * mapSizeY)) {
            return i + mapSizeX;
        }
        else {
            return -1;
        }
    }

    if (dir == 1) {
        //East
        if ((i / mapSizeX) == ((i + 1) / mapSizeX)) {
            return i + 1;
        }
        else {
            return -1;
        }
    }

    if (dir == 2) {
        //West
        if ((i / mapSizeX) == ((i - 1) / mapSizeX)) {
            return i - 1;
        }
        else {
            return -1;
        }

    }

    if (dir == 3) {
        //South
        if (i - mapSizeX > 0) {
            return i - mapSizeX;
        }
        else {
            return -1;
        }
    }

    return -2;
}

int findAsPath(iVec2 startTile, iVec2 endTile, unsigned char* mapData, int* path, int pathMaxSize) {

    int mapSizeX = mapData[0];
    int mapSizeY = mapData[1];

    int start = startTile.y * mapSizeX + startTile.x;
    int end = endTile.y * mapSizeX + endTile.x;

    int mapDataSizeInt = mapSizeX * mapSizeY; 
    int mapDataSizeByte = mapDataSizeInt * sizeof(int);
    int totalDataSizeByte = mapDataSizeByte * 4;

    int* aStarData = malloc(totalDataSizeByte);

    int* openSet = aStarData;
    int openSetSize = 0;

    openSet[0] = start;
    openSetSize += 1;

    int* cameFromIdx = aStarData + mapDataSizeInt;
    int* gScore = aStarData + (mapDataSizeInt * 2);
    int* fScore = aStarData + (mapDataSizeInt * 3);

    for (int i = mapDataSizeInt; i < (mapDataSizeInt * 4); i++) {
        aStarData[i] = -1;
    }

    gScore[start] = 0;

    while (openSetSize > 0) {

        //Get the node with the lowest fScore
        int curIdx = 0;
        int fScoreLow = fScore[openSet[0]];

        for (int i = 1; i < openSetSize; i++) {
            if (fScore[openSet[i]] < fScoreLow) {
                fScoreLow = fScore[openSet[i]];
                curIdx = i;
            }
        }

        if (openSet[curIdx] == end) {

            int pathSize = 1;
            int curIdx = end;
            while (curIdx != start) {
                curIdx = cameFromIdx[curIdx];
                pathSize += 1;
            }

            curIdx = end;
            int skip = pathSize - pathMaxSize;
            if (skip < 0) {
                skip = 0;
            }
            int writeIdx = pathSize-1;
            while (1) {
                if (skip) {
                    skip -= 1;
                }
                else {
                    path[writeIdx] = curIdx;
                }

                if (curIdx == start) {
                    break;
                }
                curIdx = cameFromIdx[curIdx];
                writeIdx -= 1;
            }

            free(aStarData);

            if (pathSize > pathMaxSize) {
                pathSize = pathMaxSize;
            }

            return pathSize; //Return the Path we found
        }

        //check the neightbours of the current node
        int curNode = openSet[curIdx];

        //swap and drop current node
        openSet[curIdx] = openSet[openSetSize - 1];
        openSetSize -= 1;

        for (int i = 0; i < 4; i++) {
            int neighIdx = getNeighbour(curNode, i, mapSizeX, mapSizeY);
            if (neighIdx == -1) {
                continue;
            }

            //int tentative_gScore = calcPathg(curNode.tileIdx,cameFromIdx,map) + map.tile[curNode.tileIdx].moveCost;
            //int curNeigh_gScore = calcPathg(neighIdx,cameFromIdx,map);


            unsigned int dataPos = 2 + curNode * 4;
            unsigned int tileData;
            memcpy(&tileData, mapData + dataPos, 4);

            int tentative_gScore = gScore[curNode] + calcTileMoveCost(tileData);
            int curNeigh_gScore = gScore[neighIdx];

            if ((tentative_gScore < curNeigh_gScore) || curNeigh_gScore == -1) {
                cameFromIdx[neighIdx] = curNode;
                gScore[neighIdx] = tentative_gScore;
                fScore[neighIdx] = tentative_gScore + hDistNodes(neighIdx, end, mapSizeX);

                // if not in openset add
                int add = 1;
                for (int i = 0; i < openSetSize; i++) {
                    if (openSet[i] == neighIdx) {
                        add = 0;
                    }
                }

                if (add) {
                    openSet[openSetSize] = neighIdx;
                    openSetSize += 1;
                }
            }

        }

    }

    free(aStarData);
    return 0;
}

TileResources calcTileResources(unsigned int tileData) {

    TileResources resources = {0};
    int divider = 1;

    if (tSWAMP & tileData) {
        divider += 1;

        resources.water += 1;
        resources.wood += 1;
        resources.animals += 0;
        resources.fish += 1;
        resources.plants += 1;
        resources.minerals += 0;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tDESERT & tileData) {
        divider += 1;

        resources.water += 0;
        resources.wood += 0;
        resources.animals += 0;
        resources.fish += 0;
        resources.plants += 1;
        resources.minerals += 1;
        resources.soil += 0;
        resources.town += 0;
    }
    if (tBARREN & tileData) {
        divider += 1;

        resources.water += 1;
        resources.wood += 1;
        resources.animals += 0;
        resources.fish += 0;
        resources.plants += 1;
        resources.minerals += 3;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tSWAMP_WATER & tileData) {
        divider += 1;

        resources.water += 2;
        resources.wood += 0;
        resources.animals += 0;
        resources.fish += 1;
        resources.plants += 1;
        resources.minerals += 0;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tLIGHT_GRASS & tileData) {
        divider += 1;

        resources.water += 0;
        resources.wood += 1;
        resources.animals += 2;
        resources.fish += 0;
        resources.plants += 1;
        resources.minerals += 1;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tDARK_GRASS & tileData) {
        divider += 1;

        resources.water += 2;
        resources.wood += 2;
        resources.animals += 3;
        resources.fish += 0;
        resources.plants += 2;
        resources.minerals += 1;
        resources.soil += 3;
        resources.town += 0;
    }
    if (tSNOW & tileData) {
        divider += 1;

        resources.water += 1;
        resources.wood += 1;
        resources.animals += 1;
        resources.fish += 0;
        resources.plants += 1;
        resources.minerals += 1;
        resources.soil += 0;
        resources.town += 0;
    }
    if (tTUNDRA & tileData) {
        divider += 1;

        resources.water += 1;
        resources.wood += 1;
        resources.animals += 0;
        resources.fish += 1;
        resources.plants += 1;
        resources.minerals += 0;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tCLIFFS_MOUNT & tileData) {
        divider += 1;

        resources.water += 1;
        resources.wood += 1;
        resources.animals += 1;
        resources.fish += 0;
        resources.plants += 1;
        resources.minerals += 3;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tWATER & tileData) {
        divider += 1;

        resources.water += 3;
        resources.wood += 0;
        resources.animals += 0;
        resources.fish += 3;
        resources.plants += 2;
        resources.minerals += 0;
        resources.soil += 2;
        resources.town += 0;     
    }
    if (tTREES & tileData) {
        divider += 1;

        resources.water += 0;
        resources.wood += 3;
        resources.animals += 3;
        resources.fish += 0;
        resources.plants += 3;
        resources.minerals += 1;
        resources.soil += 1;
        resources.town += 0;
    }
    if (tBRIDGE_HOUSE & tileData) {
        divider += 1;

        resources.water += 0;
        resources.wood += 0;
        resources.animals += 0;
        resources.fish += 0;
        resources.plants += 0;
        resources.minerals += 0;
        resources.soil += 0;
        resources.town += 3;
    }

    resources.water /= divider;
    resources.wood /= divider;
    resources.animals /= divider;
    resources.fish /= divider;
    resources.plants /= divider;
    resources.minerals /= divider;
    resources.soil /= divider;
    resources.town /= divider;
    return resources;
}

void UpdateDrawFrame(void* v_state){

    State* state = (State*)v_state;
    Rectangle cursorRec = { 0.0f,0.0f,18.0f,18.0f };

    if (IsKeyReleased(KEY_RIGHT)) state->cursTilePos.x += 1;
    if (IsKeyReleased(KEY_LEFT)) state->cursTilePos.x -= 1;
    if (IsKeyReleased(KEY_UP)) state->cursTilePos.y += 1;
    if (IsKeyReleased(KEY_DOWN)) state->cursTilePos.y -= 1;

    if (state->cursTilePos.y < 0) state->cursTilePos.y = 0;
    if (state->cursTilePos.y >= state->mapSizeY) state->cursTilePos.y = state->mapSizeY-1;
    if (state->cursTilePos.x < 0) state->cursTilePos.x = 0;
    if (state->cursTilePos.x >= state->mapSizeX) state->cursTilePos.x = state->mapSizeX - 1;

    int ScrollLockHigh = state->WagonEnt.wagonTilePos.y * state->tileSize * state->scale;
    int ScrollLockLow = ScrollLockHigh - (state->baseSizeY - state->tileSize - 1) * state->scale;

    if (IsKeyDown(KEY_W) && state->smoothScrollY <= (state->map.height - state->baseSizeY + 1)*state->scale && state->smoothScrollY< ScrollLockHigh) state->smoothScrollY += 1;
    if (IsKeyDown(KEY_S) && state->smoothScrollY>0 && state->smoothScrollY > ScrollLockLow) state->smoothScrollY -= 1;


    state->renderParams.smoothScrollY = state->smoothScrollY;

    if (state->movingWagon) {
        state->pathsize = findAsPath(state->WagonEnt.wagonTilePos, state->cursTilePos, state->mapData, state->path, 12);
        state->totalPathCost = 0;
        for (int i = 0; i < state->pathsize; i++) {
            unsigned int dataPos = 2 + state->path[i] * 4;
            unsigned int tileData;
            memcpy(&tileData, state->mapData + dataPos, 4);
            state->totalPathCost += calcTileMoveCost(tileData);
        }
    }

    if (IsKeyReleased(KEY_ENTER)) {
        if (state->movingWagon) {
            state->WagonEnt.wagonTargetTilePos = mapIdxToXY(state->path[0],state->mapSizeX);
            state->movingWagon = false;
        }
        else {
            int res = memcmp(&state->WagonEnt.wagonTilePos, &state->cursTilePos,sizeof(iVec2));
            if (res == 0) {
                state->movingWagon = true;
            }
        }
    }
        
    if (state->movingWagon == false && (state->movePathIdx < state->pathsize)) {
        if (moveWagon(&state->WagonEnt)) {
            state->movePathIdx += 1;
            if (state->movePathIdx == state->pathsize) {
                state->movePathIdx = 0;
                state->pathsize = 0;
            }
            else {
                state->WagonEnt.wagonTargetTilePos = mapIdxToXY(state->path[state->movePathIdx], state->mapSizeX);
            }
        };
    }


    unsigned int tileData = getTileData(state->cursTilePos.x, state->cursTilePos.y, state->mapData);


    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    

    BeginTextureMode(state->mapRendTex);

    ClearBackground(RAYWHITE);

    int scrollOffset = (state->smoothScrollY / state->scale);

    int mapRenderOffset = -(state->map.height - state->mapRendTex.texture.height);

    DrawTexture(state->map, 0, mapRenderOffset + scrollOffset, WHITE);

    //drawDebugGrid(renderParams);

    int centOff = (cursorRec.width - state->tileSize) / 2;
    iVec2 cursPixel = mapTileXYtoScreenXY(state->cursTilePos.x, state->cursTilePos.y, state->renderParams);


    iVec2 wagPixel = mapTileXYtoScreenXY(state->WagonEnt.wagonWorldPos.x, state->WagonEnt.wagonWorldPos.y, state->renderParams); //this is awkward, consider changing to tilepos and having the tilepos update in the move func


    if (state->pathsize) {
        for (int i = state->movePathIdx; i < state->pathsize; i++) {

            int nextTile = i + 1;
            if (nextTile >= state->pathsize) nextTile = -1;
            int prevTile = i - 1;
            if (prevTile < 0) prevTile = -1;

            int north = 0b0000000000001000;
            int east =  0b0000000000000100;
            int west =  0b0000000000000010;
            int south = 0b0000000000000001;

            int neighbors = 0;
            if (nextTile > 0 && prevTile >= 0) { //drawing a path tile, not a start or end
                if (state->path[i] + state->mapSizeX == state->path[nextTile] || state->path[i] + state->mapSizeX == state->path[prevTile]) { //tile to the north
                    neighbors |= north;
                }
                if (state->path[i] + 1 == state->path[nextTile] || state->path[i] + 1 == state->path[prevTile]) { //tile to the east
                    neighbors |= east;
                }
                if (state->path[i] - 1 == state->path[nextTile] || state->path[i] - 1 == state->path[prevTile]) { //tile to the west
                    neighbors |= west;
                }
                if (state->path[i] - state->mapSizeX == state->path[nextTile] || state->path[i] - state->mapSizeX == state->path[prevTile]) { //tile to the south
                    neighbors |= south;
                }
            }

            int drawtileID = 0; //start/end tile

            if (neighbors == (north | south)) {
                drawtileID = 1;
            }
            else if (neighbors == (east | west)) {
                drawtileID = 2;
            }
            else if (neighbors == (north | west)) {
                drawtileID = 3;
            }
            else if (neighbors == (south | east)) {
                drawtileID = 4;
            }
            else if (neighbors == (south | west)) {
                drawtileID = 5;
            }
            else if (neighbors == (north | east)) {
                drawtileID = 6;
            }
            else {
                drawtileID = -1;
            }

            Rectangle pathTileSrc = { drawtileID * state->tileSize, 2 * state->tileSize, state->tileSize,state->tileSize };

            
            
            iVec2 tileLoc = mapIdxToXY(state->path[i], state->mapSizeX);
            iVec2 pathPos = mapTileXYtoScreenXY(tileLoc.x, tileLoc.y, state->renderParams);

            unsigned int tileData = getTileData(tileLoc.x, tileLoc.y, state->mapData);
            int tileMoveCost = calcTileMoveCost(tileData);

            //DrawRectangle(pathPos.x, pathPos.y, tileSize, tileSize, ColorAlpha(RED, ((float)tileMoveCost-2.0f)/8.0f));
            DrawTextureRec(state->ui, pathTileSrc, (struct Vector2) { pathPos.x, pathPos.y }, WHITE);

            char str[2];
            //sprintf_s(str, 2, "%i", tileMoveCost);
            //DrawText(str, pathPos.x, pathPos.y, 6, RED);

            
        }
    }


    renderWagon(state->WagonEnt, state->renderParams, state->wagonAni);
    

    DrawTextureRec(state->ui, cursorRec, (struct Vector2) { cursPixel.x - centOff, cursPixel.y - centOff }, WHITE);

    

    EndTextureMode();

    BeginTextureMode(state->uiRendTex);

        int windowX = state->mapSizeX * state->tileSize;
        int windowY = 0;
        int windowSizeX = 120;
        int windowSizeY = state->baseSizeY;

        renderTileInfoWindow(state->ui,windowX,windowY,windowSizeX,windowSizeY,tileData);

    //DrawTextEx(font, "test", (struct Vector2) { 5, 5 }, fontSize* scale, 1, RED);
    EndTextureMode();

    Rectangle mapRendTexSrc = { 0, 0, state->mapRendTex.texture.width, -state->mapRendTex.texture.height };
    DrawTexturePro(state->mapRendTex.texture, mapRendTexSrc,
        (struct Rectangle) { 0, -state->scale+state->smoothScrollY%state->scale, state->baseSizeX* state->scale, state->baseSizeY* state->scale},
        (struct Vector2) { 0, 0 }, 0.0f, WHITE);
    
    Rectangle uiRendTexSrc = { 0, 0, state->uiRendTex.texture.width, -state->uiRendTex.texture.height };
    DrawTexturePro(state->uiRendTex.texture, uiRendTexSrc,
        (struct Rectangle) { 0, 0, state->baseSizeX* state->scale, state->baseSizeY* state->scale},
        (struct Vector2) { 0, 0 }, 0.0f, WHITE);

    
    
    //renderTileInfoDebug(map.width* scale + (tileSize * scale), tileSize* scale , tileData);


    
    char str[3];
    //sprintf_s(str, 3, "%i", state->totalPathCost);
    //DrawText(str, 10, 10, 32, RED);
    

    EndDrawing();
    //----------------------------------------------------------------------------------
}

int main(void)
{

    //SaveFileData("test.txt", "test", 5);
    //#if !defined(_DEBUG)
    //    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
   // #endif

    State state;

    state.tileSize = 16;
    state.baseSizeX = 600;
    state.baseSizeY = 230;
    state.scale = 2;
    // Initialization
    //--------------------------------------------------------------------------------------
    state.screenWidth = state.baseSizeX* state.scale;
    state.screenHeight = state.baseSizeY* state.scale - state.scale;

    InitWindow(state.screenWidth, state.screenHeight, "raylib [core] example - basic window");

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    state.map = LoadTexture("resources/respiteTestMap.png");
    state.ui = LoadTexture("resources/UI.png");
    state.wagon = LoadTexture("resources/wagon.png");

    //global
    font = LoadFontEx("resources/rainyhearts.ttf",fontSize,NULL,0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);



    Rectangle wagonNorth = {0.0f,0.0f,16.0f,16.0f };
    Rectangle altWagonNorth = { 16.0f,0.0f,16.0f,16.0f };

    Rectangle wagonEast = { 0.0f,16.0f,16.0f,16.0f };
    Rectangle altWagonEast = { 16.0f,16.0f,16.0f,16.0f };

    Rectangle wagonWest = { 0.0f,32.0f,16.0f,16.0f };
    Rectangle altWagonWest = { 16.0f,32.0f,16.0f,16.0f };

    Rectangle wagonSouth = { 0.0f,48.0f,16.0f,16.0f };
    Rectangle altWagonSouth = { 16.0f,48.0f,16.0f,16.0f };

    Rectangle cursorRec = { 0.0f,0.0f,18.0f,18.0f };
    Rectangle wagonCursRec = { 19.0f,0.0f,18.0f,18.0f };


    state.wagonAni = (WagonAni){ state.wagon,
        wagonNorth, altWagonNorth,
        wagonEast, altWagonEast,
        wagonWest, altWagonWest,
        wagonSouth, altWagonSouth,
    };


    // Set our game frames-per-second
    //--------------------------------------------------------------------------------------
    //ToggleFullscreen();

    state.smoothScrollY = 0;
    state.cursTilePos = (iVec2){ 15,0 };
    state.mapRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);
    state.uiRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);

    state.renderParams = (RenderParams){ state.baseSizeX ,state.baseSizeY,state.map.width,state.map.height,state.tileSize,state.smoothScrollY,state.scale };

    state.movingWagon = 0;

    state.WagonEnt = (WagonEntity){ (struct iVec2) { 15,0 },(struct Vector2) { 15.0f,0.0f },(struct iVec2) { 15,0 },0, 0.01f };
    //int path[12];
    state.pathsize = 0;
    state.movePathIdx = 0;
    state.totalPathCost = 0;

    state.mapData = LoadFileData("resources/respitetest.rspb", &state.mapDataSize);
    state.mapSizeX = state.mapData[0];
    state.mapSizeY = state.mapData[1];

    
    
    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop_arg(UpdateDrawFrame, &state, 60, 1);
    #else 
        SetTargetFPS(60); 
    // Main game loop
    while (!WindowShouldClose()){
         
        UpdateDrawFrame(&state);
    }
    #endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

