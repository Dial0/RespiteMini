

/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute raylib_compile_execute script
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2023 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#define tBRIDGE_HOUSE   0b0001000000000000
#define tTREES          0b0000100000000000
#define tWATER          0b0000010000000000
#define tCLIFFS_MOUNT   0b0000001000000000
#define tTUNDRA         0b0000000100000000
#define tSNOW           0b0000000010000000
#define tDARK_GRASS     0b0000000001000000
#define tLIGHT_GRASS    0b0000000000100000
#define tSWAMP_WATER    0b0000000000010000
#define tBARREN         0b0000000000001000
#define tDESERT         0b0000000000000100
#define tSWAMP          0b0000000000000010
#define tBASE           0b0000000000000001


#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

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

void calculatePath(iVec2 start, iVec2 end) {
    iVec2 inter = { start.x, end.y };
}

void moveWagon(WagonEntity* wagon) {

    //if we are already at the target return
    if (wagon->wagonTilePos.x == wagon->wagonTargetTilePos.x
        && wagon->wagonTilePos.y == wagon->wagonTargetTilePos.y) {
        return;
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
            wagon->aniDir = North;
        }
        else { //moving South
            wagon->aniDir = South;
        }
    }

    wagon->wagonWorldPos = Vector2Add(Vector2Scale(dir, wagon->moveSpeed), wagon->wagonWorldPos);

    wagon->aniFrames += 1;

    if (Vector2Distance(wagon->wagonWorldPos, target) <= wagon->moveSpeed){
        wagon->wagonTilePos = wagon->wagonTargetTilePos;
        wagon->wagonWorldPos.x = wagon->wagonTilePos.x;
        wagon->wagonWorldPos.y = wagon->wagonTilePos.y;
        wagon->aniFrames = 0;
    }

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

void renderTileInfo(int x, int y, unsigned int tileData) {
    int textYHeight = y;
    int textHeight = 32;
    int textSpacing = 4;

    int tileMoveCost = calcTileMoveCost(tileData);

    char str[80];

    sprintf_s(str,80, "Move Cost: %i", tileMoveCost);

    DrawText(str, x, textYHeight, textHeight, RED);
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

int main(void)
{
    const int tileSize = 16;
    const int baseSizeX = 600;
    const int baseSizeY = 230;
    const int scale = 4;
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = baseSizeX* scale;
    const int screenHeight = baseSizeY* scale - scale;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Texture2D map = LoadTexture("C:\\Users\\Ethan\\OneDrive - FabDepot\\game stuff\\Mini Respite\\respiteTestMap.png");
    Texture2D ui = LoadTexture("C:\\Users\\Ethan\\OneDrive - FabDepot\\game stuff\\Mini Respite\\ui.png");
    Texture2D wagon = LoadTexture("C:\\Users\\Ethan\\OneDrive - FabDepot\\game stuff\\Mini Respite\\wagon.png");

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


    WagonAni wagonAni = { wagon,
        wagonNorth, altWagonNorth,
        wagonEast, altWagonEast,
        wagonWest, altWagonWest,
        wagonSouth, altWagonSouth,
    };


    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    //ToggleFullscreen();

    int smoothScrollY = 0;
    iVec2 cursTilePos = { 15,0 };
    RenderTexture2D gameRendRex = LoadRenderTexture(baseSizeX, baseSizeY);
    RenderTexture2D gameRendRexTemp = LoadRenderTexture(baseSizeX, baseSizeY);

    RenderParams renderParams = { baseSizeX ,baseSizeY,map.width,map.height,tileSize,smoothScrollY,scale };

    bool movingWagon = false;
    //iVec2 wagonTilePos = ;

    WagonEntity WagonEnt = { (struct iVec2) { 15,0 },(struct Vector2) { 15.0f,0.0f },(struct iVec2) { 15,0 },0, 0.01f };

    unsigned int mapDataSize;
    unsigned char* mapData = LoadFileData("respitetest.rspb", &mapDataSize);


    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        if (IsKeyReleased(KEY_RIGHT)) cursTilePos.x += 1;
        if (IsKeyReleased(KEY_LEFT)) cursTilePos.x -= 1;
        if (IsKeyReleased(KEY_UP)) cursTilePos.y += 1;
        if (IsKeyReleased(KEY_DOWN)) cursTilePos.y -= 1;

        int ScrollLockHigh = WagonEnt.wagonTilePos.y * tileSize * scale;
        int ScrollLockLow = ScrollLockHigh - (baseSizeY - tileSize - 1) * scale;

        if (IsKeyDown(KEY_W) && smoothScrollY <= (map.height - baseSizeY + 1)*scale && smoothScrollY< ScrollLockHigh) smoothScrollY += 1;
        if (IsKeyDown(KEY_S) && smoothScrollY>0 && smoothScrollY > ScrollLockLow) smoothScrollY -= 1;


        renderParams.smoothScrollY = smoothScrollY;


        if (IsKeyReleased(KEY_ENTER)) {
            if (movingWagon) {
                WagonEnt.wagonTargetTilePos = cursTilePos;
                movingWagon = false;
            }
            else {
                int res = memcmp(&WagonEnt.wagonTilePos, &cursTilePos,sizeof(iVec2));
                if (res == 0) {
                    movingWagon = true;
                }
            }
        }
         
        moveWagon(&WagonEnt);


        unsigned int tileData = getTileData(cursTilePos.x, cursTilePos.y, mapData);

        int path[12];
        int pathsize = 0;
        if (movingWagon) {
            pathsize = findAsPath(WagonEnt.wagonTilePos, cursTilePos, mapData, path, 12);
        }
        
        

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        

        BeginTextureMode(gameRendRex);

        ClearBackground(RAYWHITE);

        int scrollOffset = (smoothScrollY / scale);

        int mapRenderOffset = -(map.height - gameRendRex.texture.height);

        DrawTexture(map, 0, mapRenderOffset + scrollOffset, WHITE);

        //drawDebugGrid(renderParams);

        int centOff = (cursorRec.width - tileSize) / 2;
        iVec2 cursPixel = mapTileXYtoScreenXY(cursTilePos.x, cursTilePos.y, renderParams);


        iVec2 wagPixel = mapTileXYtoScreenXY(WagonEnt.wagonWorldPos.x, WagonEnt.wagonWorldPos.y, renderParams); //this is awkward, consider changing to tilepos and having the tilepos update in the move func

        renderWagon(WagonEnt, renderParams, wagonAni);
        

        DrawTextureRec(ui, cursorRec, (struct Vector2) { cursPixel.x - centOff, cursPixel.y - centOff }, WHITE);


        if (movingWagon) {
            DrawTextureRec(ui, wagonCursRec, (struct Vector2) { wagPixel.x - centOff, wagPixel.y - centOff }, WHITE);
            DrawLineEx((struct Vector2) { cursPixel.x + tileSize / 2, cursPixel.y + tileSize / 2 }, (struct Vector2) { cursPixel.x + tileSize / 2, wagPixel.y + tileSize / 2 }, 4, PINK);
            DrawLineEx((struct Vector2) { wagPixel.x + tileSize / 2, wagPixel.y + tileSize / 2 }, (struct Vector2) { cursPixel.x + tileSize / 2, wagPixel.y + tileSize / 2 }, 4, PINK);
        }

        if (pathsize) {
            for (int i = 0; i < pathsize; i++) {
                iVec2 pathPos = mapTileXYtoScreenXY(mapIdxToXY(path[i], 30).x, mapIdxToXY(path[i], 30).y, renderParams);
                DrawRectangle(pathPos.x, pathPos.y, tileSize, tileSize, ColorAlpha(PINK, 0.5f));
            }
        }
        EndTextureMode();


        BeginTextureMode(gameRendRexTemp);
        DrawTexture(gameRendRex.texture, 0,0, WHITE);

        EndTextureMode();


        
        DrawTextureEx(gameRendRexTemp.texture, (struct Vector2) { 0, -scale+smoothScrollY%scale }, 0.0f, scale, WHITE);

        
        renderTileInfo(map.width*scale + 10, 10, tileData);


        DrawText("PATH", 10, 10, 32, RED);
        
        

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

