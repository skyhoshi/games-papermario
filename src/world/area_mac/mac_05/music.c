#include "mac_05.h"

EvtScript N(EVS_SetupMusic) = {
    Switch(GB_StoryProgress)
        CaseRange(STORY_CH5_WHALE_MOUTH_OPEN, STORY_CH5_ENTERED_WHALE)
            Call(SetMusic, 0, SONG_WHALE_THEME, 0, VOL_LEVEL_FULL)
        CaseRange(STORY_CH3_STAR_SPRIT_DEPARTED, STORY_CH4_STAR_SPIRIT_RESCUED)
            Call(SetMusic, 0, SONG_SHY_GUY_INVASION, 0, VOL_LEVEL_FULL)
        CaseDefault
            Call(FadeOutMusic, 0, 3000)
    EndSwitch
    Call(PlaySound, SOUND_LOOP_MAC_HARBOR_WATER)
    Call(ClearAmbientSounds, 250)
    Return
    End
};

EvtScript N(EVS_80244298) = {
    Call(SetMusic, 0, SONG_JR_TROOPA_THEME, 0, VOL_LEVEL_FULL)
    Return
    End
};

EvtScript N(EVS_802442C4) = {
    Call(FadeOutMusic, 0, 1000)
    Return
    End
};

EvtScript N(EVS_802442E8) = {
    Call(SetMusic, 0, SONG_CLUB64, 0, VOL_LEVEL_FULL)
    Return
    End
};

EvtScript N(EVS_80244314) = {
    Call(SetMusic, 0, SONG_WHALE_THEME, 0, VOL_LEVEL_FULL)
    Return
    End
};

EvtScript N(EVS_80244340) = {
    Call(FadeOutMusic, 0, 3000)
    Return
    End
};
