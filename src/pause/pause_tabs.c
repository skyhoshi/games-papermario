#include "pause_common.h"

#if VERSION_PAL
#define TABS_CURSOR_OFFSET_X (-4)
#else
#define TABS_CURSOR_OFFSET_X (0)
#endif

extern MenuPanel* gPausePanels[];
void pause_tabs_draw_stats(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_badges(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_items(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_party(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_spirits(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_map(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_draw_invis(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening);
void pause_tabs_init(MenuPanel* tab);
void pause_tabs_handle_input(MenuPanel* tab);
void pause_tabs_update(MenuPanel* tab);
void pause_tabs_cleanup(MenuPanel* tab);

#if VERSION_JP
static HudElemID gPauseTabsHIDs[2];
#else
static HudElemID gPauseTabsHIDs[6];
#endif
static s32 gPauseTabsPreviousTab;
static s32 gPauseTabsHorizScrollPos;

#if VERSION_PAL
extern HudScript HES_HeaderStats_de;
extern HudScript HES_HeaderBadges_de;
extern HudScript HES_HeaderItems_de;
extern HudScript HES_HeaderParty_de;
extern HudScript HES_HeaderSpirits_de;
extern HudScript HES_HeaderMap_de;

extern HudScript HES_HeaderStats_fr;
extern HudScript HES_HeaderBadges_fr;
extern HudScript HES_HeaderItems_fr;
extern HudScript HES_HeaderParty_fr;
extern HudScript HES_HeaderSpirits_fr;
extern HudScript HES_HeaderMap_fr;

extern HudScript HES_HeaderStats_es;
extern HudScript HES_HeaderBadges_es;
extern HudScript HES_HeaderItems_es;
extern HudScript HES_HeaderParty_es;
extern HudScript HES_HeaderSpirits_es;
extern HudScript HES_HeaderMap_es;
#endif

#if VERSION_JP
HudScript* gPauseTabsHudScripts[][2] = {
    [LANGUAGE_DEFAULT] = {
        &HES_HeaderSpirits, &HES_HeaderMap
    },
};
#elif VERSION_PAL
HudScript* gPauseTabsHudScripts[][6] = {
    [LANGUAGE_DEFAULT] = {
        &HES_HeaderStats, &HES_HeaderBadges, &HES_HeaderItems,
        &HES_HeaderParty, &HES_HeaderSpirits, &HES_HeaderMap
    },
    [LANGUAGE_DE] = {
        &HES_HeaderStats_de, &HES_HeaderBadges_de, &HES_HeaderItems_de,
        &HES_HeaderParty_de, &HES_HeaderSpirits_de, &HES_HeaderMap_de
    },
    [LANGUAGE_FR] = {
        &HES_HeaderStats_fr, &HES_HeaderBadges_fr, &HES_HeaderItems_fr,
        &HES_HeaderParty_fr, &HES_HeaderSpirits_fr, &HES_HeaderMap_fr
    },
    [LANGUAGE_ES] = {
        &HES_HeaderStats_es, &HES_HeaderBadges_es, &HES_HeaderItems_es,
        &HES_HeaderParty_es, &HES_HeaderSpirits_es, &HES_HeaderMap_es
    },
};
#else
HudScript* gPauseTabsHudScripts[][6] = {
    [LANGUAGE_DEFAULT] = {
        &HES_HeaderStats, &HES_HeaderBadges, &HES_HeaderItems,
        &HES_HeaderParty, &HES_HeaderSpirits, &HES_HeaderMap
    },
};
#endif

s8 gPauseTabsGridData[] = { 0, 1, 2, 3, 4, 5 };
u8 gPauseTabsPanelIDs[] = { 1, 2, 3, 4, 5, 6 };
u8 gPauseTabsWindowIDs[] = { WIN_PAUSE_TAB_STATS, WIN_PAUSE_TAB_BADGES, WIN_PAUSE_TAB_ITEMS, WIN_PAUSE_TAB_PARTY, WIN_PAUSE_TAB_SPIRITS, WIN_PAUSE_TAB_MAP };
u8 gPauseTabsPageWindowIDs[] = { WIN_PAUSE_STATS, WIN_PAUSE_BADGES, WIN_PAUSE_ITEMS, WIN_PAUSE_PARTNERS, WIN_PAUSE_SPIRITS, WIN_PAUSE_MAP };
MenuWindowBP gPauseTabsWindowBPs[] = {
    {
        .windowID = WIN_PAUSE_TAB_STATS,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_64,
        .fpDrawContents = pause_tabs_draw_stats,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { WINDOW_UPDATE_SHOW },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_3 }
    },
    {
        .windowID = WIN_PAUSE_TAB_BADGES,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_0,
        .fpDrawContents = pause_tabs_draw_badges,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { .func = pause_update_tab_default },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_4 }
    },
    {
        .windowID = WIN_PAUSE_TAB_ITEMS,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_0,
        .fpDrawContents = pause_tabs_draw_items,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { .func = pause_update_tab_default },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_5 }
    },
    {
        .windowID = WIN_PAUSE_TAB_PARTY,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_0,
        .fpDrawContents = pause_tabs_draw_party,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { .func = pause_update_tab_default },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_6 }
    },
    {
        .windowID = WIN_PAUSE_TAB_SPIRITS,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_0,
        .fpDrawContents = pause_tabs_draw_spirits,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { .func = pause_update_tab_default },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_7 }
    },
    {
        .windowID = WIN_PAUSE_TAB_MAP,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 7 },
        .width = 43,
        .height = 15,
        .priority = WINDOW_PRIORITY_0,
        .fpDrawContents = pause_tabs_draw_map,
        .tab = NULL,
        .parentID = WIN_PAUSE_MAIN,
        .fpUpdate = { .func = pause_update_tab_default },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_8 }
    },
    {
        .windowID = WIN_PAUSE_TAB_INVIS,
        .unk_01 = 0,
        .pos = { .x = 8, .y = 8 },
        .width = 16,
        .height = 16,
        .priority = WINDOW_PRIORITY_64,
        .fpDrawContents = pause_tabs_draw_invis,
        .tab = NULL,
        .parentID = WIN_NONE,
        .fpUpdate = { WINDOW_UPDATE_SHOW },
        .extraFlags = 0,
        .style = { .customStyle = &gPauseWS_9 }
    }
};
s32 gPauseTabsCurrentTab = 0;
s32 gPauseTabsMessages[] = {
    PAUSE_MSG_TAB_STATS,
    PAUSE_MSG_TAB_BADGES,
    PAUSE_MSG_TAB_ITEMS,
    PAUSE_MSG_TAB_PARTY,
    PAUSE_MSG_TAB_SPIRITS,
    PAUSE_MSG_TAB_MAP,
};
u8 gPauseTabsInterpTable[] = { 0, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };
s32 gPauseDoBasicWindowUpdate = true; // TODO rename (eth name)
MenuPanel gPausePanelTabs = {
    .initialized = false,
    .col = 0,
    .row = 0,
    .selected = 0,
    .state = 0,
    .numCols = 6,
    .numRows = 1,
    .numPages = 0,
    .gridData = gPauseTabsGridData,
    .fpInit = pause_tabs_init,
    .fpHandleInput = pause_tabs_handle_input,
    .fpUpdate = pause_tabs_update,
    .fpCleanup = pause_tabs_cleanup
};

void pause_tabs_draw_invis(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
}

void pause_tabs_draw_stats(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
#if VERSION_JP
    draw_msg(pause_get_menu_msg(PAUSE_MSG_17), baseX + 9, baseY + 1, 255.0 - darkening * 0.5, 0, 1);
#else
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[0], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[0], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[0], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[0], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[0], baseX + 22, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[0]);
#endif
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_STATS) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

void pause_tabs_draw_badges(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
#if VERSION_JP
    draw_msg(pause_get_menu_msg(PAUSE_MSG_18), baseX + 9, baseY + 1, 255.0 - darkening * 0.5, 0, 1);
#else
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[1], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[1], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[1], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[1], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[1], baseX + 22, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[1]);
#endif
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_BADGES) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

#if VERSION_JP
void pause_tabs_draw_items(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
    draw_msg(pause_get_menu_msg(PAUSE_MSG_19), baseX + 4, baseY + 1, 255.0 - darkening * 0.5, 0, 1);
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_ITEMS) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

void pause_tabs_draw_party(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
    draw_msg(pause_get_menu_msg(PAUSE_MSG_1A), baseX + 9, baseY + 1, 255.0 - darkening * 0.5, 0, 1);
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_PARTY) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}
#else
void pause_tabs_draw_items(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[2], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[2], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[2], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[2], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[2], baseX + 22, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[2]);
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_ITEMS) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

void pause_tabs_draw_party(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[3], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[3], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[3], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[3], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[3], baseX + 22, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[3]);
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_PARTY) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}
#endif

void pause_tabs_draw_spirits(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
#if VERSION_JP
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[0], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[0], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[0], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[0], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[0], baseX + 23, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[0]);
#else
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[4], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[4], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[4], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[4], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[4], baseX + 22, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[4]);
#endif
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_SPIRITS) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

void pause_tabs_draw_map(MenuPanel* menu, s32 baseX, s32 baseY, s32 width, s32 height, s32 opacity, s32 darkening) {
#if VERSION_JP
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[1], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[1], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[1], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[1], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[1], baseX + 23, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[1]);
#else
    if (darkening != 0) {
        hud_element_set_flags(gPauseTabsHIDs[5], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[5], 255.0 - darkening * 0.5);
    } else {
        hud_element_clear_flags(gPauseTabsHIDs[5], HUD_ELEMENT_FLAG_TRANSPARENT);
        hud_element_set_alpha(gPauseTabsHIDs[5], 255);
    }

    hud_element_set_render_pos(gPauseTabsHIDs[5], baseX + 23, baseY + 7);
    hud_element_draw_without_clipping(gPauseTabsHIDs[5]);
#endif
    if (gPauseMenuCurrentTab == 0) {
        if (gPauseTabsWindowIDs[menu->col] == WIN_PAUSE_TAB_MAP) {
            pause_set_cursor_pos(gPauseTabsWindowIDs[menu->col], baseX + TABS_CURSOR_OFFSET_X, baseY + 6);
        }
    }
}

void pause_tabs_init(MenuPanel* tab) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gPauseTabsHudScripts[0]); i++) {
        gPauseTabsHIDs[i] = hud_element_create(gPauseTabsHudScripts[gCurrentLanguage][i]);
        hud_element_set_flags(gPauseTabsHIDs[i], HUD_ELEMENT_FLAG_80);
    }

    for (i = 0; i < ARRAY_COUNT(gPauseTabsWindowBPs); i++) {
        gPauseTabsWindowBPs[i].tab = tab;
    }

    setup_pause_menu_tab(gPauseTabsWindowBPs, ARRAY_COUNT(gPauseTabsWindowBPs));
    gWindows[WIN_PAUSE_TAB_INVIS].pos.y = 25;
    gPauseTabsHorizScrollPos = 0;
    tab->initialized = true;
    gPauseTabsPreviousTab = 5;
}

void pause_tabs_handle_input(MenuPanel* tab) {
    Window* pauseWindows;
    s32 x;

    if (gPauseHeldButtons & (BUTTON_STICK_LEFT | BUTTON_Z)) {
        do {
            if (--tab->col < 0) {
                tab->col = 5;
                if (gPauseTabsHorizScrollPos < 1800) {
                    gPauseTabsHorizScrollPos += 1800;
                }
            }
        } while (gPausePanels[gPauseTabsPanelIDs[tab->col]]->initialized == false);
    }

    if (gPauseHeldButtons & (BUTTON_STICK_RIGHT | BUTTON_R)) {
        do {
            if (++tab->col >= 6) {
                tab->col = 0;
                if (gPauseTabsHorizScrollPos > 0) {
                    gPauseTabsHorizScrollPos -= 1800;
                }
            }
        } while (gPausePanels[gPauseTabsPanelIDs[tab->col]]->initialized == false);
    }

    if (tab->col != gPauseTabsCurrentTab) {
        replace_window_update(gPauseTabsWindowIDs[tab->col], 0x40, pause_update_tab_active);
        replace_window_update(gPauseTabsWindowIDs[gPauseTabsCurrentTab], 0x40, pause_update_tab_inactive);

        pauseWindows = &gWindows[WIN_PAUSE_TAB_STATS];
        x = pauseWindows[tab->col].pos.x;
        gWindows[WIN_PAUSE_TAB_INVIS].pos.x = x + 6;
        gWindows[WIN_PAUSE_TAB_INVIS].pos.y = 25;
        gPauseTabsPreviousTab = gPauseTabsCurrentTab;
        gPauseTabsCurrentTab = tab->col;
        sfx_play_sound(SOUND_MENU_CHANGE_TAB);
    }

    if ((gPausePressedButtons & BUTTON_A) && gPausePanels[gPauseTabsPanelIDs[tab->col]]->initialized == true) {
        sfx_play_sound(SOUND_MENU_NEXT);
        gPauseMenuCurrentTab = gPauseTabsPanelIDs[tab->col];
    }

    gPauseCurrentDescMsg = pause_get_menu_msg(gPauseTabsMessages[tab->col]);
    gPauseCurrentDescIconScript = NULL;
}

void pause_tabs_update(MenuPanel* tab) {
    s32 absValue;
    f32 delta;
    f32 deltaBefore;
    f32 f7;
    void (*fpUpdateInactive)(s32 windowIndex, s32* flags, s32* posX, s32* posY, s32* posZ, f32* scaleX, f32* scaleY,
                                 f32* rotX, f32* rotY, f32* rotZ, s32* darkening, s32* opacity);
    void (*fpUpdateActive)(s32 windowIndex, s32* flags, s32* posX, s32* posY, s32* posZ, f32* scaleX, f32* scaleY,
                                 f32* rotX, f32* rotY, f32* rotZ, s32* darkening, s32* opacity);
    WindowUpdateFunc fpUpdate;
    s32 i;
    s32 flag;
    s32 sgn;

    deltaBefore = tab->col * 300 - gPauseTabsHorizScrollPos;
    absValue = abs(deltaBefore);
    sgn = sign(deltaBefore);

    if (absValue >= 16) {
        delta = absValue * 0.5;
        if (delta > 32.0f) {
            delta = 32.0f;
        }
    } else {
        delta = gPauseTabsInterpTable[absValue];
    }
    delta = delta * sgn;

    gPauseTabsHorizScrollPos += delta;

    if ((gPauseTabsPreviousTab != 0 || tab->col != 5) && (gPauseTabsPreviousTab < tab->col || gPauseTabsPreviousTab == 5 && tab->col == 0)) {
        fpUpdateActive = pause_update_page_active_1;
        fpUpdateInactive = pause_update_page_inactive_1;
    } else {
        fpUpdateActive = pause_update_page_active_2;
        fpUpdateInactive = pause_update_page_inactive_2;
    }

    flag = false;
    for (i = 0; i < ARRAY_COUNT(gPauseTabsPanelIDs); i++) {
        if (gPausePanels[gPauseTabsPanelIDs[i]]->initialized && (gWindows[gPauseTabsPageWindowIDs[i]].flags & 8)) {
            flag = true;
            break;
        }
    }

    if (!flag) {
        for (i = 0; i < ARRAY_COUNT(gPauseTabsPanelIDs); i++) {
            if (gPausePanels[gPauseTabsPanelIDs[i]]->initialized) {
                fpUpdate = gWindows[gPauseTabsPageWindowIDs[i]].fpUpdate;
                if (i != tab->col && (fpUpdate.func == pause_update_page_active_1 ||
                                      fpUpdate.func == pause_update_page_active_2 ||
                                      fpUpdate.func == basic_window_update ||
                                      fpUpdate.i == 1)) {
                    set_window_update(gPauseTabsPageWindowIDs[i], (s32)fpUpdateInactive);
                    flag = true;
                }
            }
        }

        if (!flag) {
            if (gWindows[gPauseTabsPageWindowIDs[tab->col]].fpUpdate.func == pause_update_page_inactive_1 ||
                gWindows[gPauseTabsPageWindowIDs[tab->col]].fpUpdate.func == pause_update_page_inactive_2 ||
                gWindows[gPauseTabsPageWindowIDs[tab->col]].fpUpdate.i == 2) {
                if (gPauseDoBasicWindowUpdate) {
                    fpUpdateActive = &basic_window_update;
                    gPauseDoBasicWindowUpdate = false;
                }

                set_window_update(gPauseTabsPageWindowIDs[tab->col], (s32)fpUpdateActive);
            }
        }
    }
}

void pause_tabs_cleanup(MenuPanel* tab) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gPauseTabsHIDs); i++) {
        hud_element_free(gPauseTabsHIDs[i]);
    }
}
