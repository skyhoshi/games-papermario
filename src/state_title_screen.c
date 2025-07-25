#include "common.h"
#include "audio.h"
#include "audio/public.h"
#include "nu/nusys.h"
#include "hud_element.h"
#include "sprite.h"
#include "game_modes.h"
#include "ld_addrs.h"

#if VERSION_JP
#define TITLE_WIDTH 272
#define TITLE_TILE_HEIGHT 2
#define TITLE_HEIGHT 88
#define TITLE_POS_LEFT 25
#define TITLE_POS_TOP 34
#define FILENAME_ERROR "\x54\x77\xA2\xF7\xF7\xf7\xf7\xf7"
#else
// Properties of the title screen Paper Mario logo
#define TITLE_WIDTH  200 // Width of the texture
#define TITLE_HEIGHT 112 // Height of the texture
#define TITLE_TILE_HEIGHT 2 // Height of an individually loaded tile in the texture
#define TITLE_POS_LEFT ((SCREEN_WIDTH - TITLE_WIDTH) / 2) // Left edge of the texture on screen
#define TITLE_POS_TOP 15 // Top edge of the texture on screen (with no offset)
#define FILENAME_ERROR "ERROR\xf7\xf7\xf7"
#endif

#define TITLE_NUM_TILES (TITLE_HEIGHT / TITLE_TILE_HEIGHT) // Number of tiles in the texture
#define TITLE_TILE_PIXELS (TITLE_WIDTH * TITLE_TILE_HEIGHT) // Number of pixels in a single tile of the texture

#if VERSION_PAL
#define TITLE_LOGO_YOFFSET -110
#define TITLE_START_TIME 400
#else
#define TITLE_LOGO_YOFFSET -100
#define TITLE_START_TIME 480
#endif


enum TitleScreenStates {
    TITLE_STATE_INIT            = 0x00000000,
    TITLE_STATE_APPEAR          = 0x00000001,
    TITLE_STATE_HOLD            = 0x00000002,   // show the title screen with PRESS START blinking
    TITLE_STATE_UNUSED          = 0x00000003,
    TITLE_STATE_BEGIN_DISMISS   = 0x00000004,
    TITLE_STATE_DISMISS         = 0x00000005,
    TITLE_STATE_EXIT            = 0x00000006,
};

enum {
    NEXT_STATE_DEMO             = 10,
    NEXT_STATE_INTRO            = 9,
    NEXT_STATE_FILE_SELECT      = 5,
    NEXT_STATE_NONE             = 0, // used only for the initial value
};

enum TitleMenuVisibilityStates {
    TITLEMENU_STATE_FADE_IN,
#if VERSION_PAL
    TITLEMENU_STATE_VISIBLE,
#endif
    TITLEMENU_STATE_FADE_OUT,
};

s16 TitleScreenNextState = NEXT_STATE_NONE;

SaveMetadata gSaveSlotMetadata[4] = {
    { .filename = { FILENAME_ERROR } },
    { .filename = { FILENAME_ERROR } },
    { .filename = { FILENAME_ERROR } },
    { .filename = { FILENAME_ERROR } },
};

u8 gSaveSlotHasData[4] = { true, true, true, true };

s32 TitleMenu_Alpha = 0; // the opacity of "PRESS START" text
s32 TitleMenu_Visibility = TITLEMENU_STATE_FADE_IN; // toggles the visibility of "PRESS START"
s32 TitleMenu_BlinkCounter = 0; // counts to 16, then toggles TitleMenu_Visibility

// controls whether the intro story or the demo will player after TITLE_STATE_HOLD is done
// since this state is reached for the first time after the intro has already played once or was skipped,
// this is initially false and the demo is will play first.
s32 PlayIntroNext = false;

#if VERSION_PAL
bool LanguageSelectedPrev = false;
bool LanguageSelected = false;
#endif

Lights1 D_80077A38 = gdSPDefLights1(255, 255, 255, 0, 0, 0, 0, 0, 0);

Gfx TitleSetupGfx[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_CLD_SURF, G_RM_CLD_SURF2),
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsDPSetTextureFilter(G_TF_POINT),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetAlphaDither(G_AD_DISABLE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPNoOp(),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsSPEndDisplayList(),
};

#if VERSION_PAL
s32 StartGame_Width[] = { 96,   88,   144,  120 };
s32 Languages_Width[] = { 88,   80,    64,   64 };
s32 StartGame_PosX[] = { 116,  120,   88,  106 };
s32 Languages_PosX[] = { 121,  124,  130,  132 };
#endif

typedef struct TitleDataStruct {
    /* 0x0 */ s32 logo;
    /* 0x4 */ s32 copyright;
    /* 0x8 */ s32 pressStart;
    /* 0xC */ s32 copyrightPalette;
} TitleDataStruct; // size = 0x10

typedef struct TitleMenuDataStruct {
    /* 0x0 */ IMG_BIN *startGame;
    /* 0x4 */ IMG_BIN *languages;
} TitleMenuDataStruct; // size = 0x8

#if VERSION_JP
#define COPYRIGHT_WIDTH 128
#else
#define COPYRIGHT_WIDTH 144
#endif

BSS s16 TitleScreen_AppearDelay;
BSS TitleDataStruct* TitleScreen_ImgList;
BSS s32* TitleScreen_ImgList_Logo;
BSS u8 (*TitleScreen_ImgList_Copyright)[COPYRIGHT_WIDTH];
BSS s32* TitleScreen_ImgList_PressStart;
#if VERSION_PAL
BSS TitleMenuDataStruct* TitleMenu_ImgList;
BSS u8 *TitleMenu_ImgList_StartGame;
BSS u8 *TitleMenu_ImgList_Languages;
BSS s16 TitleScreen_TimeLeft;
BSS s32 StartGame_Alpha;
BSS s32 Languages_Alpha;
#else
BSS s32* TitleScreen_ImgList_CopyrightPalette;
BSS s16 TitleScreen_TimeLeft;
#endif

void appendGfx_title_screen(void);
void draw_title_screen_NOP(void);
void title_screen_draw_images(f32, f32);
void title_screen_draw_logo(f32);
void title_screen_draw_menu(void);
void title_screen_draw_copyright(f32);

void state_init_title_screen(void) {
    s32 titleDataSize;
    void* titleDataDst;
    void* titleData;

    gOverrideFlags = 0;
    gTimeFreezeMode = TIME_FREEZE_NONE;
    D_8014C248 = true;
    general_heap_create();
    clear_printers();
    sfx_set_reverb_mode(0);
    gGameStatusPtr->startupState = TITLE_STATE_INIT;
    gGameStatusPtr->logoTime = 0;
    gGameStatusPtr->context = CONTEXT_WORLD;
    gGameStatusPtr->introPart = INTRO_PART_NONE;
    startup_fade_screen_update();
    titleData = load_asset_by_name("title_data", &titleDataSize);
    titleDataDst = TitleScreen_ImgList = heap_malloc(titleDataSize);
    decode_yay0(titleData, titleDataDst);
    general_heap_free(titleData);

    TitleScreen_ImgList_Logo = (s32*)(TitleScreen_ImgList->logo + (s32) TitleScreen_ImgList);
    TitleScreen_ImgList_Copyright = (u8 (*)[COPYRIGHT_WIDTH]) ((s32*)(TitleScreen_ImgList->copyright + (s32) TitleScreen_ImgList));
    TitleScreen_ImgList_PressStart = (s32*)(TitleScreen_ImgList->pressStart + (s32) TitleScreen_ImgList);
#if VERSION_JP
    TitleScreen_ImgList_CopyrightPalette = (s32*)(TitleScreen_ImgList->copyrightPalette + (s32) TitleScreen_ImgList);
#endif
#if VERSION_PAL
    TitleMenu_ImgList = heap_malloc((s32)titlemenu_DATA_SIZE);
    dma_copy(titlemenu_ROM_START, titlemenu_ROM_END, TitleMenu_ImgList);
    TitleMenu_ImgList_StartGame = (u8*)(TitleMenu_ImgList[gCurrentLanguage].startGame + (s32) TitleMenu_ImgList);
    TitleMenu_ImgList_Languages = (u8*)(TitleMenu_ImgList[gCurrentLanguage].languages + (s32) TitleMenu_ImgList);
#endif

    create_cameras();
    gCameras[CAM_DEFAULT].updateMode = CAM_UPDATE_NO_INTERP;
    gCameras[CAM_DEFAULT].needsInit = true;
    gCameras[CAM_DEFAULT].nearClip = CAM_NEAR_CLIP;
    gCameras[CAM_DEFAULT].farClip = CAM_FAR_CLIP;
    gCurrentCameraID = CAM_DEFAULT;
    gCameras[CAM_DEFAULT].vfov = 25.0f;
    gCameras[CAM_DEFAULT].flags |= CAMERA_FLAG_DISABLED;
    gCameras[CAM_BATTLE].flags |= CAMERA_FLAG_DISABLED;
    gCameras[CAM_TATTLE].flags |= CAMERA_FLAG_DISABLED;
    gCameras[CAM_HUD].flags |= CAMERA_FLAG_DISABLED;
    set_cam_viewport(CAM_DEFAULT, 12, 28, 296, 184);
    gCameras[CAM_DEFAULT].params.basic.dist = 40;
    gCameras[CAM_DEFAULT].bgColor[0] = 0;
    gCameras[CAM_DEFAULT].bgColor[1] = 0;
    gCameras[CAM_DEFAULT].bgColor[2] = 0;
    gCameras[CAM_DEFAULT].lookAt_obj_target.x = 25.0f;
    gCameras[CAM_DEFAULT].lookAt_obj_target.y = 25.0f;
    gCameras[CAM_DEFAULT].params.basic.skipRecalc = false;
    gCameras[CAM_DEFAULT].params.basic.fovScale = 100;
    gCameras[CAM_DEFAULT].params.basic.pitch = 0;
    gCameras[CAM_DEFAULT].lookAt_eye.x = 500.0f;
    gCameras[CAM_DEFAULT].lookAt_eye.y = 1000.0f;
    gCameras[CAM_DEFAULT].lookAt_eye.z = 1500.0f;
    gCameras[CAM_DEFAULT].lookAt_obj_target.z = 150.0f;
    clear_script_list();
    clear_worker_list();
    clear_render_tasks();
    spr_init_sprites(PLAYER_SPRITES_MARIO_WORLD);
    clear_animator_list();
    clear_entity_models();
    clear_npcs();
    hud_element_clear_cache();
    reset_background_settings();
    clear_entity_data(true);
    clear_effect_data();
    gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    clear_player_data();
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    game_mode_set_fpDrawAuxUI(0, appendGfx_title_screen);
    load_map_bg("title_bg");
    set_background(&gBackgroundImage);
    bgm_set_song(0, SONG_MAIN_THEME, 0, 500, VOL_LEVEL_FULL);
    TitleScreen_TimeLeft = TITLE_START_TIME;
}

void state_step_title_screen(void) {
    s32 temp;
    u32 pressedButtons = gGameStatusPtr->pressedButtons[0];

    set_curtain_scale(1.0f);
    set_curtain_fade(0.0f);

    if (TitleScreen_TimeLeft > 0) {
        TitleScreen_TimeLeft--;
    }

    switch (gGameStatusPtr->startupState) {
        case TITLE_STATE_INIT:
            TitleScreen_AppearDelay = 3;
            gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
            gGameStatusPtr->titleScreenDismissTime = 20;
            gGameStatusPtr->titleScreenTimer = gGameStatusPtr->titleScreenDismissTime;
            gGameStatusPtr->startupState++;
            break;
        case TITLE_STATE_APPEAR:
            if (TitleScreen_AppearDelay != 0) {
                TitleScreen_AppearDelay--;
                break;
            }
            if (gGameStatusPtr->titleScreenTimer != 0) {
                gGameStatusPtr->titleScreenTimer--;
            }
            gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
            if (startup_fade_screen_in(6) != 0) {
                if (gGameStatusPtr->titleScreenTimer == 0) {
                    gGameStatusPtr->startupState = TITLE_STATE_HOLD;
                }
            }
            startup_fade_screen_update();
            break;
        case TITLE_STATE_HOLD:
#if VERSION_PAL
            if(gGameStatusPtr->pressedButtons[0] & BUTTON_STICK_DOWN) {
                LanguageSelected = true;
            }

            if(gGameStatusPtr->pressedButtons[0] & BUTTON_STICK_UP) {
                LanguageSelected = false;
            }

            if(LanguageSelectedPrev != LanguageSelected) {
                LanguageSelectedPrev = LanguageSelected;
                sfx_play_sound(SOUND_MENU_CHANGE_TAB);
                if(TitleScreen_TimeLeft < 125) {
                    TitleScreen_TimeLeft = 125;
                }
            }
#endif
            if (PlayIntroNext && TitleScreen_TimeLeft == 120) {
                bgm_set_song(0, AU_SONG_NONE, 0, 3900, VOL_LEVEL_FULL);
            }
            if (TitleScreen_TimeLeft == 0) {
                gGameStatusPtr->startupState = TITLE_STATE_BEGIN_DISMISS;
                if (!PlayIntroNext) {
                    TitleScreenNextState = NEXT_STATE_DEMO;
                } else {
                    TitleScreenNextState = NEXT_STATE_INTRO;
                }
                PlayIntroNext ^= 1;
                return;
            }
            if (pressedButtons & (BUTTON_A | BUTTON_START)) {
                gGameStatusPtr->startupState = TITLE_STATE_BEGIN_DISMISS;
                TitleScreenNextState = NEXT_STATE_FILE_SELECT;
                sfx_play_sound(SOUND_FILE_MENU_IN);
                bgm_set_song(0, SONG_FILE_SELECT, 0, 500, VOL_LEVEL_FULL);
                return;
            }
            break;
        case TITLE_STATE_BEGIN_DISMISS:
            gGameStatusPtr->startupState = TITLE_STATE_DISMISS;
            startup_set_fade_screen_color(208);
            if (TitleScreenNextState == NEXT_STATE_INTRO || TitleScreenNextState == NEXT_STATE_DEMO) {
                gGameStatusPtr->titleScreenDismissTime = 20;
            } else {
                gGameStatusPtr->titleScreenDismissTime = 10;
            }
            gGameStatusPtr->titleScreenTimer = gGameStatusPtr->titleScreenDismissTime;
            break;
        case TITLE_STATE_DISMISS:
            if (TitleScreenNextState == NEXT_STATE_INTRO || TitleScreenNextState == NEXT_STATE_DEMO) {
                if (gGameStatusPtr->titleScreenTimer != 0) {
                    gGameStatusPtr->titleScreenTimer--;
                }
                if (startup_fade_screen_out(10) != 0) {
                    if (gGameStatusPtr->titleScreenTimer == 0) {
                        gGameStatusPtr->titleScreenTimer = 3;
                        gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
                        gGameStatusPtr->startupState = TITLE_STATE_EXIT;
                    }
                }
                startup_fade_screen_update();
            } else if (TitleScreenNextState == NEXT_STATE_FILE_SELECT) {
                if (gGameStatusPtr->titleScreenTimer == 0) {
                    gGameStatusPtr->titleScreenTimer = 3;
                    gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
                    gGameStatusPtr->startupState = TITLE_STATE_EXIT;
                } else {
                    gGameStatusPtr->titleScreenTimer--;
                }
            } else {
                gGameStatusPtr->titleScreenTimer = 3;
                gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
                gGameStatusPtr->startupState = TITLE_STATE_EXIT;
            }
            break;
        case TITLE_STATE_EXIT:
            if (gGameStatusPtr->titleScreenTimer != 0) {
                gGameStatusPtr->titleScreenTimer--;
                break;
            }
            general_heap_create();
            clear_render_tasks();
            create_cameras();
            clear_entity_models();
            clear_animator_list();
            clear_npcs();
            hud_element_clear_cache();
            spr_init_sprites(PLAYER_SPRITES_MARIO_WORLD);
            clear_entity_data(true);
            clear_windows();
            gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
            gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
            gGameStatusPtr->entryID = 0;

            switch (TitleScreenNextState) {
                case NEXT_STATE_INTRO:
                    gGameStatusPtr->introPart = INTRO_PART_0;
                    set_game_mode(GAME_MODE_INTRO);
                    break;
                case NEXT_STATE_DEMO:
                    set_game_mode(GAME_MODE_DEMO);
                    break;
                case NEXT_STATE_FILE_SELECT:
                    gGameStatusPtr->areaID = AREA_KMR;
                    gGameStatusPtr->mapID = 0xB; //TODO hardcoded map IDs
                    gGameStatusPtr->entryID = 0;
#if VERSION_PAL
                    if(LanguageSelected) {
                        set_game_mode(GAME_MODE_LANGUAGE_SELECT);
                    } else {
                        set_game_mode(GAME_MODE_FILE_SELECT);
                    }
#else
                    set_game_mode(GAME_MODE_FILE_SELECT);
#endif
                    break;
            }
            return;
    }

    if (!(gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) {
        update_npcs();
        update_cameras();
    }
}

void state_drawUI_title_screen(void) {
    switch (gGameStatusPtr->startupState) {
        case TITLE_STATE_INIT:
            TitleMenu_Alpha = 0;
            TitleMenu_Visibility = TITLEMENU_STATE_FADE_IN;
            TitleMenu_BlinkCounter = 0;
#if VERSION_PAL
            StartGame_Alpha = 0;
            Languages_Alpha = 0;
#endif
            draw_title_screen_NOP();
            break;
        case TITLE_STATE_HOLD:
            draw_title_screen_NOP();
            if (gGameStatusPtr->contBitPattern & 1) {
                title_screen_draw_menu();
            }
        case TITLE_STATE_UNUSED:
            break;
        case TITLE_STATE_APPEAR:
#if VERSION_PAL
            draw_title_screen_NOP();
            break;
#endif
        case TITLE_STATE_BEGIN_DISMISS:
        case TITLE_STATE_DISMISS:
#if VERSION_PAL
            if (gGameStatusPtr->contBitPattern & 1) {
                TitleMenu_Visibility = TITLEMENU_STATE_FADE_OUT;
            }
            title_screen_draw_menu();
#endif
            draw_title_screen_NOP();
            break;
    }
}

void appendGfx_title_screen(void) {
    f32 phi_f12;
    s32 temp;

    switch (gGameStatusPtr->startupState) {
        case TITLE_STATE_INIT:
        case TITLE_STATE_UNUSED:
            break;
        case TITLE_STATE_APPEAR:
            phi_f12 = gGameStatusPtr->titleScreenTimer;
            phi_f12 /= gGameStatusPtr->titleScreenDismissTime;
            phi_f12 = SQ(phi_f12);
            title_screen_draw_images(phi_f12, phi_f12);
            break;
        case TITLE_STATE_HOLD:
            title_screen_draw_images(0.0f, 0.0f);
            break;
        case TITLE_STATE_BEGIN_DISMISS:
            title_screen_draw_images(0.0f, 0.0f);
            break;
        case TITLE_STATE_DISMISS:
            temp = gGameStatusPtr->titleScreenTimer - 1;
            phi_f12 = gGameStatusPtr->titleScreenDismissTime - temp;
            phi_f12 /= gGameStatusPtr->titleScreenDismissTime;
            phi_f12 = SQ(phi_f12);
            title_screen_draw_images(phi_f12, phi_f12);
            break;
    }

    gDPPipeSync(gMainGfxPos++);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));
    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPPipeSync(gMainGfxPos++);
    gSPClearGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING |
                            G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH);
    gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH);
    gSPSetLights1(gMainGfxPos++, D_80077A38);

    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE);
    gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);
    gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE);
    gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP);
    gDPSetTextureConvert(gMainGfxPos++, G_TC_FILT);
    gDPSetCombineKey(gMainGfxPos++, G_CK_NONE);
    gDPSetAlphaCompare(gMainGfxPos++, G_AC_NONE);
    render_frame(false);
    render_frame(true);
}

void draw_title_screen_NOP(void) {
}

void title_screen_draw_images(f32 logoMoveAlpha, f32 copyrightMoveAlpha) {
    title_screen_draw_logo(logoMoveAlpha);
    title_screen_draw_copyright(copyrightMoveAlpha);
}

void title_screen_draw_logo(f32 moveAlpha) {
    s32 yOffset;
    s32 i;

    gSPDisplayList(gMainGfxPos++, TitleSetupGfx);
    gDPPipeSync(gMainGfxPos++);
    yOffset = TITLE_LOGO_YOFFSET * moveAlpha;

    for (i = 0; i < TITLE_NUM_TILES; i++) {
        // Load a tile from the logo texture
        gDPLoadTextureTile(gMainGfxPos++, &TitleScreen_ImgList_Logo[i * TITLE_TILE_PIXELS], G_IM_FMT_RGBA, G_IM_SIZ_32b,
                           TITLE_WIDTH, TITLE_TILE_HEIGHT, // width, height
                           0, 0, (TITLE_WIDTH - 1), (TITLE_TILE_HEIGHT - 1), // uls, ult, lrs, lrt
                           0, // pal
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, // cms, cmt
                           G_TX_NOMASK, G_TX_NOMASK, // masks, maskt
                           G_TX_NOLOD, G_TX_NOLOD); // shifts, shiftt
        // Draw a scissored texture rectangle with the loaded tile
        gSPScisTextureRectangle(gMainGfxPos++,
            (TITLE_POS_LEFT)                                                      << 2, // ulx
            (TITLE_POS_TOP + TITLE_TILE_HEIGHT * i + yOffset)                     << 2, // uly
            (TITLE_POS_LEFT + TITLE_WIDTH)                                        << 2, // lrx
            (TITLE_POS_TOP + TITLE_TILE_HEIGHT + TITLE_TILE_HEIGHT * i + yOffset) << 2, // lry
            G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    }

    gDPPipeSync(gMainGfxPos++);
}

#if VERSION_IQUE
#define VAR_1 20
#define VAR_2 628
#else
#define VAR_1 32
#define VAR_2 676
#endif

void title_screen_draw_menu(void) {
#if VERSION_PAL
    switch (TitleMenu_Visibility) {
        case TITLEMENU_STATE_FADE_IN:
            TitleMenu_Alpha += 80;
            if (TitleMenu_Alpha > 255) {
                TitleMenu_Alpha = 255;
                TitleMenu_Visibility = TITLEMENU_STATE_VISIBLE;
            }
            /* fallthrough */
        case TITLEMENU_STATE_VISIBLE:
            if (!LanguageSelected) {
                StartGame_Alpha += 64;
                Languages_Alpha -= 64;
                if (StartGame_Alpha > TitleMenu_Alpha) {
                    StartGame_Alpha = TitleMenu_Alpha;
                }
                if (Languages_Alpha < TitleMenu_Alpha / 2.0f) {
                    Languages_Alpha = TitleMenu_Alpha / 2.0f;
                }
            } else {
                Languages_Alpha += 64;
                StartGame_Alpha -= 64;
                if (Languages_Alpha > TitleMenu_Alpha) {
                    Languages_Alpha = TitleMenu_Alpha;
                }
                if (StartGame_Alpha < TitleMenu_Alpha / 2.0f) {
                    StartGame_Alpha = TitleMenu_Alpha / 2.0f;
                }
            }

            break;
        case TITLEMENU_STATE_FADE_OUT:
            TitleMenu_Alpha -= 64;
            if (TitleMenu_Alpha < 0) {
                TitleMenu_Alpha = 0;
            }
            break;
    }
    if (TitleMenu_Visibility != TITLEMENU_STATE_VISIBLE) {
        if (!LanguageSelected) {
            StartGame_Alpha = TitleMenu_Alpha;
            Languages_Alpha = TitleMenu_Alpha / 2.0f;
        } else {
            Languages_Alpha = TitleMenu_Alpha;
            StartGame_Alpha = TitleMenu_Alpha / 2.0f;
        }
    }
#else
    switch (TitleMenu_Visibility) {
        case TITLEMENU_STATE_FADE_IN:
            TitleMenu_Alpha -= 128;
            if (TitleMenu_Alpha < 0) {
                TitleMenu_Alpha = 0;
            }

            TitleMenu_BlinkCounter++;
            if (TitleMenu_BlinkCounter >= 16) {
                TitleMenu_BlinkCounter = 0;
                TitleMenu_Visibility = TITLEMENU_STATE_FADE_OUT;
            }
            break;
        case TITLEMENU_STATE_FADE_OUT:
            TitleMenu_Alpha += 128;
            if (TitleMenu_Alpha > 255) {
                TitleMenu_Alpha = 255;
            }

            TitleMenu_BlinkCounter++;
            if (TitleMenu_BlinkCounter >= 16) {
                TitleMenu_BlinkCounter = 0;
                TitleMenu_Visibility = TITLEMENU_STATE_FADE_IN;
            }
    }
#endif

    gSPDisplayList(gMainGfxPos++, TitleSetupGfx);
    gDPSetCombineMode(gMainGfxPos++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
#if VERSION_PAL
    gDPSetPrimColor(gMainGfxPos++, 0, 0, 248, 240, 152, (u8)StartGame_Alpha);
    gDPPipeSync(gMainGfxPos++);

    gDPLoadTextureBlock(gMainGfxPos++, TitleMenu_ImgList_StartGame, G_IM_FMT_IA, G_IM_SIZ_8b,
                        StartGame_Width[gCurrentLanguage], 16, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, StartGame_PosX[gCurrentLanguage] * 4, 149 * 4,
                        (StartGame_PosX[gCurrentLanguage] + StartGame_Width[gCurrentLanguage]) * 4, (149 + 16) * 4,
                        G_TX_RENDERTILE, 0, 0, 0x0400, 0x0400);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, 248, 240, 152, (u8)Languages_Alpha);
    gDPPipeSync(gMainGfxPos++);

    gDPLoadTextureBlock(gMainGfxPos++, TitleMenu_ImgList_Languages, G_IM_FMT_IA, G_IM_SIZ_8b,
                        Languages_Width[gCurrentLanguage], 16, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, Languages_PosX[gCurrentLanguage] * 4, 169 * 4,
                        (Languages_PosX[gCurrentLanguage] + Languages_Width[gCurrentLanguage]) * 4, (169 + 16) * 4,
                        G_TX_RENDERTILE, 0, 0, 0x0400, 0x0400);
#else
    gDPSetPrimColor(gMainGfxPos++, 0, 0, 248, 240, 152, TitleMenu_Alpha);
    gDPPipeSync(gMainGfxPos++);
    gDPLoadTextureBlock(gMainGfxPos++, TitleScreen_ImgList_PressStart, G_IM_FMT_IA, G_IM_SIZ_8b, 128, VAR_1, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, 384, 548, 896, VAR_2, G_TX_RENDERTILE, 0, 0, 0x0400, 0x0400);
#endif
    gDPPipeSync(gMainGfxPos++);
}

#if VERSION_IQUE
#define RECT_SIZE 0x28
#define YL_BASE 724
#define YH_BASE 764
#define COPYRIGHT_TEX_CHUNKS 4
#define COPYRIGHT_IMG(k, i) &TitleScreen_ImgList_Copyright[k]
#define LTT_LRT 9
#else
#define RECT_SIZE 0x40
#define YL_BASE 764
#define YH_BASE 828
#define COPYRIGHT_TEX_CHUNKS 2
#define COPYRIGHT_IMG(k, i) &TitleScreen_ImgList_Copyright[16 * i]
#define LTT_LRT 15
#endif

void title_screen_draw_copyright(f32 moveAlpha) {
    s32 alpha;
    s32 i;

    gSPDisplayList(gMainGfxPos++, &TitleSetupGfx);
#if VERSION_JP
    gDPSetTextureLUT(gMainGfxPos++, G_TT_RGBA16);
#endif
    gDPPipeSync(gMainGfxPos++);

    alpha = 255.0f - (moveAlpha * 255.0f);
    if (alpha < 255) {
        if (alpha < 0) {
            alpha = 0;
        }
        gDPSetCombineMode(gMainGfxPos++, PM_CC_02, PM_CC_02);
        gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, alpha);
    }

#if VERSION_JP
    gDPLoadTLUT_pal16(gMainGfxPos++, 0, TitleScreen_ImgList_CopyrightPalette);
    gDPLoadTextureTile_4b(gMainGfxPos++, TitleScreen_ImgList_Copyright, G_IM_FMT_CI,
                          COPYRIGHT_WIDTH, 0, 0, 0, 127, 31, 0,
                          G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                          G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, 388, YL_BASE, 900, 892, G_TX_RENDERTILE,
                        0, 0, 0x0400, 0x0400);
#else
    for (i = 0; i < COPYRIGHT_TEX_CHUNKS; i++) {
        s32 k = 10 * i;
        alpha = 0; // TODO figure out why this is needed

        gDPLoadTextureTile(gMainGfxPos++, COPYRIGHT_IMG(k, i), G_IM_FMT_IA, G_IM_SIZ_8b,
                           COPYRIGHT_WIDTH, 32, 0, 0, 143, LTT_LRT, 0,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                           G_TX_NOLOD);
        gSPTextureRectangle(gMainGfxPos++, 356, YL_BASE + (RECT_SIZE * i), 932, YH_BASE + (RECT_SIZE * i),
                            G_TX_RENDERTILE, 0, 0, 0x0400, 0x0400);
    }
#endif
    gDPPipeSync(gMainGfxPos++);
}
