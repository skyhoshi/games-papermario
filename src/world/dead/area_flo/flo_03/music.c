#include "flo_03.h"

EvtScript N(EVS_SetupMusic) = {
    Call(GetEntryID, LVar0)
    IfEq(LVar0, flo_03_ENTRY_2)
        Call(SetMusic, 0, SONG_SUNSHINE_RETURNS, 0, VOL_LEVEL_FULL)
    Else
        Switch(GB_StoryProgress)
            CaseLe(STORY_CH6_ASKED_TO_DEFEAT_MONTY_MOLES)
                IfEq(GF_FLO03_DefeatedAll_MontyMoles, false)
                    Call(SetMusic, 0, SONG_MONTY_MOLE_ASSAULT, 0, VOL_LEVEL_FULL)
                Else
                    Call(SetMusic, 0, SONG_FLOWER_FIELDS_CLOUDY, 0, VOL_LEVEL_FULL)
                EndIf
            CaseLt(STORY_CH6_DESTROYED_PUFF_PUFF_MACHINE)
                Call(SetMusic, 0, SONG_FLOWER_FIELDS_CLOUDY, 0, VOL_LEVEL_FULL)
            CaseDefault
                Call(SetMusic, 0, SONG_FLOWER_FIELDS_SUNNY, 0, VOL_LEVEL_FULL)
        EndSwitch
    EndIf
    Return
    End
};

EvtScript N(EVS_PushFlowerSong) = {
    IfGe(GB_StoryProgress, STORY_CH6_ASKED_TO_DEFEAT_MONTY_MOLES)
        IfEq(GF_FLO03_DefeatedAll_MontyMoles, true)
            Call(PushSong, SONG_FLOWER_NPC_THEME, 0)
        EndIf
    EndIf
    Return
    End
};

EvtScript N(EVS_PopSong) = {
    IfGe(GB_StoryProgress, STORY_CH6_ASKED_TO_DEFEAT_MONTY_MOLES)
        IfEq(GF_FLO03_DefeatedAll_MontyMoles, true)
            Call(FadeOutMusic, 0, 250)
            Wait(10)
            Call(PopSong)
        EndIf
    EndIf
    Return
    End
};
