

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

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

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

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

//Returns the screenXY of the top left pixel of the tile
iVec2 mapTileXYtoScreenXY(int MapX, int MapY, RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* param.tileSize, (MapY + tileYOffset)* param.tileSize + mapRenderOffset % param.tileSize + scrollOffset };
}

iVec2 mapWorldXYtoScreenXY(float MapX, float MapY, RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* (float)(param.tileSize), (MapY + tileYOffset)* (float)(param.tileSize) + mapRenderOffset % param.tileSize + scrollOffset };
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

enum MoveDir { North, South, East, West };

typedef struct WagonEntity {
    iVec2 wagonTilePos;
    Vector2 wagonWorldPos;
    iVec2 wagonTargetTilePos;
    int aniFrames;
    float moveSpeed;
    enum MoveDir aniDir;
} WagonEntity;


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

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        if (IsKeyReleased(KEY_RIGHT)) cursTilePos.x += 1;
        if (IsKeyReleased(KEY_LEFT)) cursTilePos.x -= 1;
        if (IsKeyReleased(KEY_UP)) cursTilePos.y -= 1;
        if (IsKeyReleased(KEY_DOWN)) cursTilePos.y += 1;

        int ScrollLockHigh = -WagonEnt.wagonTilePos.y * tileSize * scale;
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


        EndTextureMode();


        BeginTextureMode(gameRendRexTemp);
        DrawTexture(gameRendRex.texture, 0,0, WHITE);
        EndTextureMode();

        DrawTextureEx(gameRendRexTemp.texture, (struct Vector2) { 0, -scale+smoothScrollY%scale }, 0.0f, scale, WHITE);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

