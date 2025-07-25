#include "common.h"
#include "script_api/battle.h"
#include "battle/action_cmd/jump.h"
#include "effects.h"
#include "sprite/player.h"

#define NAMESPACE battle_move_earthquake_jump

#include "battle/common/move/JumpSupport.inc.c"

API_CALLABLE(N(func_802A10E4_785C04)) {
    script->varTable[0] = 3;
    return ApiStatus_DONE2;
}

MATCHING_BSS(0x3CC0);

Difficulty1D N(DifficultyTable) = {
    11, 10, 9, 8, 7, 6, 5, 4
};

extern EvtScript N(EVS_UseMove_Basic);
extern EvtScript N(EVS_UseMove_Super);
extern EvtScript N(EVS_UseMove_Ultra);

EvtScript N(EVS_UseMove) = {
    Set(LFlagA, false)
    Call(ShowActionHud, true)
    Call(GetMenuSelection, LVar0, LVar1, LVar2)
    Switch(LVar1)
        CaseEq(0)
            ExecWait(N(EVS_UseMove_Basic))
        CaseEq(1)
            ExecWait(N(EVS_UseMove_Super))
        CaseEq(2)
            ExecWait(N(EVS_UseMove_Ultra))
    EndSwitch
    Return
    End
};

EvtScript N(EVS_UseMove_Basic) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetGoalToFirstTarget, ACTOR_SELF)
    Call(GetGoalPos, ACTOR_SELF, LVar0, LVar1, LVar2)
    Sub(LVar0, 20)
    Call(InitTargetIterator)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(GetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Sub(LVar3, Float(70.0))
    IfLt(LVar0, LVar3)
        Sub(LVar0, 20)
        Set(LVar3, LVar0)
    EndIf
    Call(SetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Call(AddGoalPos, ACTOR_PLAYER, -30, 0, 0)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_ATTACK_APPROACH)
    Call(SetActorSpeed, ACTOR_PLAYER, Float(5.0))
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Run)
    Call(CancelablePlayerRunToGoal, 0, LVar0)
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Idle)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(AddGoalPos, ACTOR_PLAYER, 30, 0, 0)
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Set(LVarB, LVarA)
    Add(LVarB, 2)
    Call(action_command_jump_start, LVarB, AC_DIFFICULTY_STANDARD)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_MarioB1_Stomp, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    ChildThread
        Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
        Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
    EndChildThread
    ChildThread
        Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
        Add(LVar0, 24)
        Add(LVar1, 10)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
    EndChildThread
    Wait(10)
    Call(InitTargetIterator)
    Call(GetPlayerActionQuality, LVarB)
    Set(LVar9, 0)
    Label(1)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
        IfEq(LVar0, HIT_RESULT_MISS)
            Goto(2)
        EndIf
        Switch(LVarB)
            CaseGt(false)
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
            CaseDefault
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
        EndSwitch
        Label(2)
        Call(ChooseNextTarget, ITER_NEXT, LVar0)
        Add(LVar9, 1)
        Call(GetTargetListLength, LVar0)
        IfLt(LVar9, LVar0)
            Goto(1)
        EndIf
    Switch(LVarC)
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
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(GetJumpActionQuality, LVarE)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
        ChildThread
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_JUMP_FINISH)
            Wait(5)
            Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
        EndChildThread
        Call(InterruptActionCommand)
        Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
        Call(LoadActionCommand, ACTION_COMMAND_JUMP)
        Call(action_command_jump_init)
        Set(LVarA, 26)
        Switch(LVarF)
            CaseEq(0)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(1)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(2)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseEq(3)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseDefault
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
        EndSwitch
        Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Fall, ANIM_Mario1_SpinFall)
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
        IfEq(LVarF, 0)
            Call(PlayerBasicJumpToGoal, 24, PLAYER_BASIC_JUMP_3)
        Else
            Call(PlayerBasicJumpToGoal, 24, PLAYER_BASIC_JUMP_4)
        EndIf
        ChildThread
            Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
            Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
        EndChildThread
        ChildThread
            Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
            Add(LVar0, 24)
            Add(LVar1, 10)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
        EndChildThread
        Wait(10)
        Call(GetCommandAutoSuccess, LVar1)
        IfEq(LVar1, 1)
            IfGt(LVarF, 3)
                Set(LFlag0, true)
            EndIf
        EndIf
        Set(LVar0, 3)
        Call(N(func_802A10E4_785C04))
        IfGt(LVarF, LVar0)
            Set(LFlag0, true)
        EndIf
        Call(InitTargetIterator)
        Call(GetPlayerActionQuality, LVarB)
        Set(LVar9, 0)
        Label(11)
            Call(SetGoalToTarget, ACTOR_PLAYER)
            Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
            IfEq(LVar0, HIT_RESULT_MISS)
                Goto(12)
            EndIf
            Switch(LVarB)
                CaseGt(false)
                    IfEq(LFlag0, false)
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_NICE_HIT)
                    Else
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS | BS_FLAGS1_NICE_HIT | BS_FLAGS1_NO_RATING)
                    EndIf
                CaseDefault
                    Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_1, SOUND_NONE)
                    Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS)
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
            Label(12)
            Call(ChooseNextTarget, ITER_NEXT, LVar0)
            Add(LVar9, 1)
            Call(GetTargetListLength, LVar0)
            IfLt(LVar9, LVar0)
                Goto(11)
            EndIf
        Switch(LVarC)
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
        Goto(10)
    Return
    End
};

EvtScript N(EVS_UseMove_Super) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetGoalToFirstTarget, ACTOR_SELF)
    Call(GetGoalPos, ACTOR_SELF, LVar0, LVar1, LVar2)
    Sub(LVar0, 20)
    Call(InitTargetIterator)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(GetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Sub(LVar3, Float(70.0))
    IfLt(LVar0, LVar3)
        Sub(LVar0, 20)
        Set(LVar3, LVar0)
    EndIf
    Call(SetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Call(AddGoalPos, ACTOR_PLAYER, -30, 0, 0)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_ATTACK_APPROACH)
    Call(SetActorSpeed, ACTOR_PLAYER, Float(5.0))
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Run)
    Call(CancelablePlayerRunToGoal, 0, LVar0)
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Idle)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(AddGoalPos, ACTOR_PLAYER, 30, 0, 0)
    ExecWait(N(EVS_CheckForAPress))
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_BeforeJump)
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Set(LVarB, LVarA)
    Add(LVarB, 2)
    Call(action_command_jump_start, LVarB, AC_DIFFICULTY_STANDARD)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_MarioB1_Stomp, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    ChildThread
        Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
        Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
    EndChildThread
    ChildThread
        Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
        Add(LVar0, 24)
        Add(LVar1, 10)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
    EndChildThread
    Wait(10)
    Call(InitTargetIterator)
    Call(GetPlayerActionQuality, LVarB)
    Set(LVar9, 0)
    Label(1)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
        IfEq(LVar0, HIT_RESULT_MISS)
            Goto(2)
        EndIf
        Switch(LVarB)
            CaseGt(false)
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
            CaseDefault
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
        EndSwitch
        Label(2)
        Call(ChooseNextTarget, ITER_NEXT, LVar0)
        Add(LVar9, 1)
        Call(GetTargetListLength, LVar0)
        IfLt(LVar9, LVar0)
            Goto(1)
        EndIf
    Switch(LVarC)
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
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(GetJumpActionQuality, LVarE)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
        ChildThread
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_JUMP_FINISH)
            Wait(5)
            Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
        EndChildThread
        Call(InterruptActionCommand)
        Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
        Call(LoadActionCommand, ACTION_COMMAND_JUMP)
        Call(action_command_jump_init)
        Set(LVarA, 39)
        Switch(LVarF)
            CaseEq(0)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(1)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(2)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseEq(3)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseDefault
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
        EndSwitch
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
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
        ChildThread
            Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
            Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
        EndChildThread
        ChildThread
            Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
            Add(LVar0, 24)
            Add(LVar1, 10)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
        EndChildThread
        Wait(10)
        Call(GetCommandAutoSuccess, LVar1)
        IfEq(LVar1, 1)
            IfGt(LVarF, 4)
                Set(LFlag0, true)
            EndIf
        EndIf
        Set(LVar0, 4)
        Call(N(func_802A10E4_785C04))
        IfGt(LVarF, LVar0)
            Set(LFlag0, true)
        EndIf
        Call(InitTargetIterator)
        Call(GetPlayerActionQuality, LVarB)
        Set(LVar9, 0)
        Label(11)
            Call(SetGoalToTarget, ACTOR_PLAYER)
            Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
            IfEq(LVar0, HIT_RESULT_MISS)
                Goto(12)
            EndIf
            Switch(LVarB)
                CaseGt(false)
                    IfEq(LFlag0, false)
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_NICE_HIT)
                    Else
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS | BS_FLAGS1_NICE_HIT | BS_FLAGS1_NO_RATING)
                    EndIf
                CaseDefault
                    Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_2, SOUND_NONE)
                    Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS)
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
            Label(12)
            Call(ChooseNextTarget, ITER_NEXT, LVar0)
            Add(LVar9, 1)
            Call(GetTargetListLength, LVar0)
            IfLt(LVar9, LVar0)
                Goto(1) // @bug? shouldn't this be Goto(11)?
            EndIf
        Switch(LVarC)
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
        Goto(10)
    Return
    End
};

EvtScript N(EVS_UseMove_Ultra) = {
    Call(LoadActionCommand, ACTION_COMMAND_JUMP)
    Call(action_command_jump_init)
    Call(SetGoalToFirstTarget, ACTOR_SELF)
    Call(GetGoalPos, ACTOR_SELF, LVar0, LVar1, LVar2)
    Sub(LVar0, 20)
    Call(InitTargetIterator)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(GetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Sub(LVar3, Float(70.0))
    IfLt(LVar0, LVar3)
        Sub(LVar0, 20)
        Set(LVar3, LVar0)
    EndIf
    Call(SetGoalPos, ACTOR_PLAYER, LVar3, LVar4, LVar5)
    Call(AddGoalPos, ACTOR_PLAYER, -30, 0, 0)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_ATTACK_APPROACH)
    Call(SetActorSpeed, ACTOR_PLAYER, Float(5.0))
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Run)
    Call(CancelablePlayerRunToGoal, 0, LVar0)
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Idle)
    Call(SetGoalToTarget, ACTOR_PLAYER)
    Call(AddGoalPos, ACTOR_PLAYER, 30, 0, 0)
    ExecWait(N(EVS_CheckForAPress))
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_BeforeJump)
    ExecWait(N(EVS_JumpSupport_CalcJumpTime))
    Set(LVarB, LVarA)
    Add(LVarB, 2)
    Call(action_command_jump_start, LVarB, AC_DIFFICULTY_STANDARD)
    Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_MIDAIR)
    Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
    Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_MarioB1_Stomp, ANIM_MarioB1_Stomp)
    Call(PlayerBasicJumpToGoal, LVarA, PLAYER_BASIC_JUMP_0)
    ChildThread
        Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
        Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
        Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
    EndChildThread
    ChildThread
        Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
        Add(LVar0, 24)
        Add(LVar1, 10)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
        PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
    EndChildThread
    Wait(10)
    Call(InitTargetIterator)
    Call(GetPlayerActionQuality, LVarB)
    Set(LVar9, 0)
    Label(1)
        Call(SetGoalToTarget, ACTOR_PLAYER)
        Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
        IfEq(LVar0, HIT_RESULT_MISS)
            Goto(2)
        EndIf
        Switch(LVarB)
            CaseGt(false)
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_NICE_HIT)
            CaseDefault
                Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_INCLUDE_POWER_UPS | BS_FLAGS1_TRIGGER_EVENTS)
        EndSwitch
        Label(2)
        Call(ChooseNextTarget, ITER_NEXT, LVar0)
        Add(LVar9, 1)
        Call(GetTargetListLength, LVar0)
        IfLt(LVar9, LVar0)
            Goto(1)
        EndIf
    Switch(LVarC)
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
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
        Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
    EndChildThread
    Call(GetJumpActionQuality, LVarE)
    Set(LVarF, 0)
    Set(LFlag0, false)
    Label(10)
        ChildThread
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_PRE_ULTRA_JUMP_FINISH)
            Wait(5)
            Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
            Call(UseBattleCamPreset, BTL_CAM_PLAYER_JUMP_FINISH)
        EndChildThread
        Call(InterruptActionCommand)
        Call(SetActionDifficultyTable, Ref(N(DifficultyTable)))
        Call(LoadActionCommand, ACTION_COMMAND_JUMP)
        Call(action_command_jump_init)
        Set(LVarA, 27)
        Switch(LVarF)
            CaseEq(0)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(1)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_STANDARD)
            CaseEq(2)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseEq(3)
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARDER)
            CaseDefault
                Call(action_command_jump_start, LVarA, AC_DIFFICULTY_HARD)
        EndSwitch
        Call(SetGoalPos, ACTOR_PLAYER, 30, 0, 0)
        Call(EnablePlayerBlur, ACTOR_BLUR_ENABLE)
        Call(SetJumpAnimations, ACTOR_PLAYER, 0, ANIM_Mario1_Jump, ANIM_Mario1_Jump, ANIM_Mario1_SpinFall)
        IfEq(LVarF, 0)
            Call(PlayerUltraJumpToGoal, 25, PLAYER_ULTRA_JUMP_2)
        Else
            Call(PlayerUltraJumpToGoal, 25, PLAYER_ULTRA_JUMP_4)
        EndIf
        Call(EnablePlayerBlur, ACTOR_BLUR_DISABLE)
        ChildThread
            Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.2))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 10, Float(2.0))
            Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.0))
            Call(ShakeCam, CAM_BATTLE, 0, 3, Float(0.7))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.4))
            Call(ShakeCam, CAM_BATTLE, 0, 6, Float(0.1))
            Call(ShakeCam, CAM_BATTLE, 0, 4, Float(0.05))
        EndChildThread
        ChildThread
            Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
            Add(LVar0, 24)
            Add(LVar1, 10)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 0, 30, 0, 0, 0, 0, 0)
            PlayEffect(EFFECT_SMOKE_IMPACT, 0, LVar0, LVar1, LVar2, 72, 8, 24, 30, 0, 0, 0, 0, 0)
        EndChildThread
        Wait(10)
        Call(GetCommandAutoSuccess, LVar1)
        IfEq(LVar1, 1)
            IfGt(LVarF, 5)
                Set(LFlag0, true)
            EndIf
        EndIf
        Set(LVar0, 5)
        Call(N(func_802A10E4_785C04))
        IfGt(LVarF, LVar0)
            Set(LFlag0, true)
        EndIf
        Call(InitTargetIterator)
        Call(GetPlayerActionQuality, LVarB)
        Set(LVar9, 0)
        Label(11)
            Call(SetGoalToTarget, ACTOR_PLAYER)
            Call(PlayerTestEnemy, LVar0, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, 0, 0, 16)
            IfEq(LVar0, HIT_RESULT_MISS)
                Goto(12)
            EndIf
            Switch(LVarB)
                CaseGt(false)
                    IfEq(LFlag0, false)
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_NICE_HIT)
                    Else
                        Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                        Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS | BS_FLAGS1_NICE_HIT | BS_FLAGS1_NO_RATING)
                    EndIf
                CaseDefault
                    Call(SetActorSounds, ACTOR_PLAYER, ACTOR_SOUND_HURT, SOUND_ACTOR_JUMPED_3, SOUND_NONE)
                    Call(PlayerDamageEnemy, LVarC, DAMAGE_TYPE_QUAKE | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT | DAMAGE_TYPE_MULTIPLE_POPUPS, 0, 0, 1, BS_FLAGS1_TRIGGER_EVENTS)
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
            Label(12)
            Call(ChooseNextTarget, ITER_NEXT, LVar0)
            Add(LVar9, 1)
            Call(GetTargetListLength, LVar0)
            IfLt(LVar9, LVar0)
                Goto(1)  // @bug? shouldn't this be Goto(11)?
            EndIf
        Switch(LVarC)
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
        Goto(10)
    Return
    End
};
