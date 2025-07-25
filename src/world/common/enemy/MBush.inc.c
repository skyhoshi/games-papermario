#include "MBush.h"
#include "sprite/player.h"

EvtScript N(EVS_NpcAI_MBush) = {
    Call(EnableNpcShadow, NPC_SELF, false)
    Call(SetNpcAnimation, NPC_SELF, ANIM_MBush_Anim00)
    Call(SetSelfVar, 0, 0)
    Label(0)
    Call(GetSelfVar, 0, LVar0)
    IfEq(LVar0, 0)
        Wait(1)
        Goto(0)
    EndIf
    SetGroup(EVT_GROUP_NEVER_PAUSE)
    Call(SetTimeFreezeMode, TIME_FREEZE_PARTIAL)
    Call(DisablePlayerInput, true)
    Call(PlaySoundAtNpc, NPC_SELF, SOUND_SEARCH_BUSH, SOUND_SPACE_DEFAULT)
    Call(GetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Call(SetSelfVar, 10, LVar0)
    Call(SetSelfVar, 11, LVar1)
    Call(SetSelfVar, 12, LVar2)
    Add(LVar0, 2)
    Call(SetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Wait(1)
    Sub(LVar0, 3)
    Call(SetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Wait(1)
    Add(LVar0, 2)
    Call(SetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Wait(1)
    Sub(LVar0, 3)
    Call(SetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Wait(1)
    Add(LVar0, 2)
    Call(SetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Wait(1)
    Thread
        Wait(10)
        Call(SetNpcAnimation, NPC_SELF, ANIM_MBush_Anim03)
    EndThread
    Thread
        Wait(6)
        Call(InterpPlayerYaw, 90, 0)
    EndThread
    Call(SetNpcFlagBits, NPC_SELF, NPC_FLAG_IGNORE_PLAYER_COLLISION, true)
    Call(EnableNpcShadow, NPC_SELF, true)
    Call(GetPlayerPos, LVar0, LVar1, LVar2)
    Add(LVar0, 25)
    Sub(LVar2, 5)
    Call(NpcJump1, NPC_SELF, LVar0, LVar1, LVar2, 15)
    Wait(4)
    Call(SetNpcAnimation, NPC_SELF, ANIM_MBush_Anim07)
    Wait(2)
    Call(SetPlayerAnimation, ANIM_Mario1_Flail)
    Call(SetTimeFreezeMode, TIME_FREEZE_NONE)
    Call(DisablePlayerInput, false)
    Call(StartBattle)
}; // fallthrough :(

EvtScript N(EVS_NpcInteract_MBush) = {
    Call(SetSelfVar, 0, 1)
    Return
    End
};

EvtScript N(EVS_NpcDefeat_MBush) = {
    Call(GetBattleOutcome, LVar0)
    Switch(LVar0)
        CaseEq(OUTCOME_PLAYER_WON)
            Call(DoNpcDefeat)
        CaseEq(OUTCOME_PLAYER_FLED)
            Call(SetNpcAnimation, NPC_SELF, ANIM_MBush_Anim05)
            Call(GetSelfVar, 10, LVar0)
            Call(GetSelfVar, 11, LVar1)
            Call(GetSelfVar, 12, LVar2)
            Call(NpcJump1, NPC_SELF, LVar0, LVar1, LVar2, 8)
            Call(EnableNpcShadow, NPC_SELF, false)
            Call(SetNpcAnimation, NPC_SELF, ANIM_MBush_Anim00)
            Call(SetNpcFlagBits, NPC_SELF, NPC_FLAG_IGNORE_PLAYER_COLLISION, false)
            Call(BindNpcAI, NPC_SELF, Ref(N(EVS_NpcAI_MBush)))
        CaseEq(OUTCOME_ENEMY_FLED)
            Call(SetEnemyFlagBits, NPC_SELF, ENEMY_FLAG_FLED, true)
            Call(RemoveNpc, NPC_SELF)
    EndSwitch
    Return
    End
};

NpcSettings N(NpcSettings_MBush) = {
    .height = 30,
    .radius = 30,
    .level = ACTOR_LEVEL_M_BUSH,
    .onInteract = &N(EVS_NpcInteract_MBush),
    .ai = &N(EVS_NpcAI_MBush),
    .onDefeat = &N(EVS_NpcDefeat_MBush),
};

#define MBUSH_FLAGS \
    BASE_PASSIVE_FLAGS | ENEMY_FLAG_USE_INSPECT_ICON | ENEMY_FLAG_DO_NOT_AUTO_FACE_PLAYER
