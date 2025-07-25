#include "common.h"
#include "script_api/battle.h"
#include "battle/action_cmd/jump.h"
#include "battle/battle.h"
#include "sprite/player.h"

#define NAMESPACE battle_move_power_bounce

#include "battle/common/move/JumpSupport.inc.c"

Difficulty1D N(DifficultyTable) = {
    7, 6, 5, 4, 3, 2, 1, 0
};

s32 N(BaseHitChance) = 200;

API_CALLABLE(N(GetHitChance)) {
    Actor* targetActor = get_actor(get_actor(script->owner1.actorID)->targetActorID);
    s32 powerBounceChance;

    script->varTable[0] = 99;
    powerBounceChance = targetActor->actorBlueprint->powerBounceChance;
    if (is_ability_active(ABILITY_DODGE_MASTER)) {
        powerBounceChance += 7;
    }
    N(BaseHitChance) = N(BaseHitChance) * powerBounceChance / 100;

    if (N(BaseHitChance) < rand_int(100)) {
        script->varTable[0] = 0;
    }

    return ApiStatus_DONE2;
}

BSS s32 N(HitCounter);

API_CALLABLE(N(InitializeHitCounter)) {
    N(HitCounter) = 0;
    return ApiStatus_DONE2;
}

API_CALLABLE(N(IncrementHitCounter)) {
    N(HitCounter)++;
    return ApiStatus_DONE2;
}

API_CALLABLE(N(StoreHitCountRecord)) {
    PlayerData* playerData = &gPlayerData;

    if (playerData->powerBounces < N(HitCounter)) {
        playerData->powerBounces = N(HitCounter);
    }

    return ApiStatus_DONE2;
}

extern EvtScript N(EVS_UseMove_Basic);
extern EvtScript N(EVS_UseMove_Super);
extern EvtScript N(EVS_UseMove_Ultra);

EvtScript N(EVS_UseMove) = {
    Set(LFlagA, false)
    Call(ShowActionHud, true)
    Call(N(InitializeHitCounter))
    Call(GetMenuSelection, LVar0, LVar1, LVar2)
    Switch(LVar1)
        CaseEq(0)
            Set(LVarC, 1)
            ExecWait(N(EVS_UseMove_Basic))
        CaseEq(1)
            Set(LVarC, 2)
            ExecWait(N(EVS_UseMove_Super))
        CaseEq(2)
            Set(LVarC, 3)
            ExecWait(N(EVS_UseMove_Ultra))
    EndSwitch
    Call(N(StoreHitCountRecord))
    Return
    End
};

EvtScript N(EVS_UseMove_Basic) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
    ExecWait(N(EVS_JumpSupport_Approach))
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Fall, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_JUMP, 0, 0, 1, 0)
    IfEq(LVar0, HIT_RESULT_MISS)
        ExecWait(N(EVS_JumpSupport_Miss))
        Return
    EndIf
    Wait(1)
    Call(GetPlayerActionQuality, LVar0)
    Switch(LVar0)
        CaseGt(false)
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
        CaseDefault
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
    EndSwitch
    Switch(LVar0)
        CaseOrEq(HIT_RESULT_HIT)
        CaseOrEq(HIT_RESULT_NO_DAMAGE)
            ExecWait(N(EVS_JumpSupport_G))
            Return
        EndCaseGroup
        CaseOrEq(HIT_RESULT_NICE)
        CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
        EndCaseGroup
    EndSwitch
    ChildThread
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_JUMP_FINISH)
        Wait(5)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(GetJumpActionQuality, LVarE)
    Call(N(IncrementHitCounter))
    Set(LVarD, 0)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
        ChildThread
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_JUMP_FINISH)
            Wait(5)
            Call(SetGoalToTarget, ACTOR_PLAYER)
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
        EndChildThread
        Call(InterruptActionCommand)
        Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
        Call(LoadActionCommand, ACTION_COMMAND_JUMP)
        Call(action_command_jump_init)
        Set(LVarA, 24)
        Switch(LVarF)
            CaseEq(0)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
            CaseEq(1)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_EASY)
            CaseEq(2)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(3)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseDefault
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
        EndSwitch
        Sub(LVarD, 1)
        Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Fall, ANIM_Mario1_SpinFall)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        IfEq(LVarF, 0)
            Call(PlayerBasicJumpToGoal, 24, PLAYER_BASIC_JUMP_3)
        Else
            Call(PlayerBasicJumpToGoal, 24, PLAYER_BASIC_JUMP_4)
        EndIf
        Call(GetCommandAutoSuccess, LVar1)
        IfEq(LVar1, 1)
            IfGe(LVarF, 3)
                Set(LFlag0, true)
            EndIf
        EndIf
        Call(N(GetHitChance))
        IfGe(LVarF, LVar0)
            Set(LFlag0, true)
        EndIf
        Wait(1)
        Call(GetPlayerActionQuality, LVar0)
        Switch(LVar0)
            CaseGt(0)
                IfEq(LFlag0, false)
                    Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                    Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, BS_FLAGS1_NICE_HIT)
                Else
                    Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                    Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, BS_FLAGS1_TRIGGER_EVENTS | BS_FLAGS1_NICE_HIT | BS_FLAGS1_NO_RATING)
                EndIf
            CaseDefault
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, BS_FLAGS1_TRIGGER_EVENTS)
        EndSwitch
        Switch(LVarF)
            CaseEq(0)
                Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_1)
            CaseEq(1)
                Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_2)
            CaseEq(2)
                Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_3)
            CaseEq(3)
                Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
            CaseDefault
                Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
        EndSwitch
        Call(SetActionResult, LVarE)
        Switch(LVar0)
            CaseOrEq(HIT_RESULT_HIT)
            CaseOrEq(HIT_RESULT_NO_DAMAGE)
                IfEq(LFlag0, true)
                    ExecWait(N(EVS_JumpSupport_Rebound))
                    Return
                EndIf
                ExecWait(N(EVS_JumpSupport_G))
                Return
            EndCaseGroup
            CaseOrEq(HIT_RESULT_NICE)
            CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
                IfEq(LFlag0, true)
                    ExecWait(N(EVS_JumpSupport_Rebound))
                    Return
                EndIf
            EndCaseGroup
        EndSwitch
        Add(LVarF, 1)
        Call(N(IncrementHitCounter))
        Goto(10)
    Return
    End
};

EvtScript N(EVS_UseMove_Super) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
    ExecWait(N(EVS_JumpSupport_Approach))
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Fall, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_JUMP, 0, 0, 1, 0)
    IfEq(LVar0, HIT_RESULT_MISS)
        ExecWait(N(EVS_JumpSupport_Miss))
        Return
    EndIf
    Wait(1)
    Call(GetPlayerActionQuality, LVar0)
    Switch(LVar0)
        CaseGt(false)
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
        CaseDefault
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
    EndSwitch
    Switch(LVar0)
        CaseOrEq(HIT_RESULT_HIT)
        CaseOrEq(HIT_RESULT_NO_DAMAGE)
            ExecWait(N(EVS_JumpSupport_G))
            Return
        EndCaseGroup
        CaseOrEq(HIT_RESULT_NICE)
        CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
        EndCaseGroup
    EndSwitch
    Call(GetJumpActionQuality, LVarE)
    Call(N(IncrementHitCounter))
    Set(LVarD, 0)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
    ChildThread
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_JUMP_FINISH)
        Wait(5)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(InterruptActionCommand)
    Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Set(LVarA, 37)
    Switch(LVarF)
        CaseEq(0)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
        CaseEq(1)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_EASY)
        CaseEq(2)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
        CaseEq(3)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
        CaseDefault
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
    EndSwitch
    Sub(LVarD, 1)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Sit, ANIM_Mario1_SpinJump)
    Call(EnablePlayerBlur, ACTOR_BLUR_ENABLE)
    IfEq(LVarF, 0)
        Call(PlayerSuperJumpToGoal, 20, PLAYER_SUPER_JUMP_3)
        Wait(7)
        Call(PlayerSuperJumpToGoal, 3, PLAYER_SUPER_JUMP_6)
    Else
        Call(PlayerSuperJumpToGoal, 20, PLAYER_SUPER_JUMP_4)
        Wait(7)
        Call(PlayerSuperJumpToGoal, 3, PLAYER_SUPER_JUMP_5)
    EndIf
    Call(EnablePlayerBlur, ACTOR_BLUR_DISABLE)
    Call(GetCommandAutoSuccess, LVar1)
    IfEq(LVar1, 1)
        IfGt(LVarF, 3)
            Set(LFlag0, true)
        EndIf
    EndIf
    Call(N(GetHitChance))
    IfGt(LVarF, LVar0)
        Set(LFlag0, true)
    EndIf
    Wait(1)
    Call(GetPlayerActionQuality, LVar0)
    Switch(LVar0)
        CaseGt(false)
            IfEq(LFlag0, false)
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
                Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 64)
            Else
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
                Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 224)
            EndIf
        CaseDefault
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
            Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 32)
    EndSwitch
    Switch(LVarF)
        CaseEq(0)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_1)
        CaseEq(1)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_2)
        CaseEq(2)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_3)
        CaseEq(3)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
        CaseDefault
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
    EndSwitch
    Call(SetActionResult, LVarE)
    Switch(LVar0)
        CaseOrEq(HIT_RESULT_HIT)
        CaseOrEq(HIT_RESULT_NO_DAMAGE)
            IfEq(LFlag0, true)
                ExecWait(N(EVS_JumpSupport_Rebound))
                Return
            EndIf
            ExecWait(N(EVS_JumpSupport_G))
            Return
        EndCaseGroup
        CaseOrEq(HIT_RESULT_NICE)
        CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
            IfEq(LFlag0, true)
                ExecWait(N(EVS_JumpSupport_Rebound))
                Return
            EndIf
        EndCaseGroup
    EndSwitch
    Add(LVarF, 1)
    Call(N(IncrementHitCounter))
    Goto(10)
    Return
    End
};

EvtScript N(EVS_UseMove_Ultra) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
    ExecWait(N(EVS_JumpSupport_Approach))
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Fall, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_JUMP, 0, 0, 1, 0)
    IfEq(LVar0, HIT_RESULT_MISS)
        ExecWait(N(EVS_JumpSupport_Miss))
        Return
    EndIf
    Wait(1)
    Call(GetPlayerActionQuality, LVar0)
    Switch(LVar0)
        CaseGt(false)
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
        CaseDefault
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
            Call(PlayerDamageEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
    EndSwitch
    Switch(LVar0)
        CaseOrEq(HIT_RESULT_HIT)
        CaseOrEq(HIT_RESULT_NO_DAMAGE)
            ExecWait(N(EVS_JumpSupport_G))
            Return
        EndCaseGroup
        CaseOrEq(HIT_RESULT_NICE)
        CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
        EndCaseGroup
    EndSwitch
    Call(GetJumpActionQuality, LVarE)
    Call(N(IncrementHitCounter))
    Set(LVarD, 0)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
    ChildThread
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_ULTRA_JUMP_FINISH)
        Wait(5)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(InterruptActionCommand)
    Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Set(LVarA, 25)
    Switch(LVarF)
        CaseEq(0)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_VERY_EASY)
        CaseEq(1)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_EASY)
        CaseEq(2)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
        CaseEq(3)
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
        CaseDefault
            Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
    EndSwitch
    Sub(LVarD, 1)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(EnablePlayerBlur, ACTOR_BLUR_ENABLE)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Jump, ANIM_Mario1_SpinFall)
    IfEq(LVarF, 0)
        Call(PlayerUltraJumpToGoal, 25, PLAYER_ULTRA_JUMP_2)
    Else
        Call(PlayerUltraJumpToGoal, 25, PLAYER_ULTRA_JUMP_4)
    EndIf
    Call(EnablePlayerBlur, ACTOR_BLUR_DISABLE)
    Call(GetCommandAutoSuccess, LVar1)
    IfEq(LVar1, 1)
        IfGt(LVarF, 3)
            Set(LFlag0, true)
        EndIf
    EndIf
    Call(N(GetHitChance))
    IfGt(LVarF, LVar0)
        Set(LFlag0, true)
    EndIf
    Wait(1)
    Call(GetPlayerActionQuality, LVar0)
    Switch(LVar0)
        CaseGt(false)
            IfEq(LFlag0, false)
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 64)
            Else
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 224)
            EndIf
        CaseDefault
            Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
            Call(PlayerPowerBounceEnemy, LVar0, DAMAGE_TYPE_JUMP | DAMAGE_TYPE_POWER_BOUNCE, 0, 0, LVarC, LVarD, 32)
    EndSwitch
    Switch(LVarF)
        CaseEq(0)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_1)
        CaseEq(1)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_2)
        CaseEq(2)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_3)
        CaseEq(3)
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
        CaseDefault
            Call(PlaySoundAtActor, ACTOR_PLAYER, SOUND_JUMP_COMBO_4)
    EndSwitch
    Call(SetActionResult, LVarE)
    Switch(LVar0)
        CaseOrEq(HIT_RESULT_HIT)
        CaseOrEq(HIT_RESULT_NO_DAMAGE)
            IfEq(LFlag0, true)
                ExecWait(N(EVS_JumpSupport_Rebound))
                Return
            EndIf
            ExecWait(N(EVS_JumpSupport_G))
            Return
        EndCaseGroup
        CaseOrEq(HIT_RESULT_NICE)
        CaseOrEq(HIT_RESULT_NICE_NO_DAMAGE)
            IfEq(LFlag0, true)
                ExecWait(N(EVS_JumpSupport_Rebound))
                Return
            EndIf
        EndCaseGroup
    EndSwitch
    Add(LVarF, 1)
    Call(N(IncrementHitCounter))
    Goto(10)
    Return
    End
};
