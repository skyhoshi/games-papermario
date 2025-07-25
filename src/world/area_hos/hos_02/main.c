#include "hos_02.h"

EvtScript N(EVS_ExitStarBeam) = {
    SetGroup(EVT_GROUP_EXIT_MAP)
    Call(DisablePlayerInput, true)
    Call(GetPartnerInUse, LVar0)
    IfNe(LVar0, PARTNER_NONE)
        Call(InterruptUsePartner)
        Wait(20)
        Loop(0)
            Wait(1)
            Call(IsPlayerOnValidFloor, LVar0)
            IfEq(LVar0, true)
                BreakLoop
            EndIf
        EndLoop
    EndIf
    Call(DisablePlayerPhysics, true)
    Call(DisablePartnerAI, 0)
    ExecWait(N(EVS_DescendStarBeam))
    Call(GotoMap, Ref("hos_01"), hos_01_ENTRY_1)
    Wait(100)
    Return
    End
};

EvtScript N(EVS_ExitWalk_hos_03_0) = EVT_EXIT_WALK(60, hos_02_ENTRY_1, "hos_03", hos_03_ENTRY_0);

EvtScript N(EVS_BindExitTriggers) = {
    Call(GetEntryID, LVar0)
    IfNe(LVar0, hos_02_ENTRY_0)
        BindTrigger(Ref(N(EVS_ExitStarBeam)), TRIGGER_FLOOR_TOUCH, COLLIDER_deiliw, 1, 0)
    EndIf
    BindTrigger(Ref(N(EVS_ExitWalk_hos_03_0)), TRIGGER_FLOOR_ABOVE, COLLIDER_deilie, 1, 0)
    Return
    End
};

EvtScript N(EVS_EnterStarBeam) = {
    Call(DisablePlayerInput, true)
    Call(DisablePlayerPhysics, true)
    Call(DisablePartnerAI, 0)
    Call(SetNpcFlagBits, NPC_PARTNER, NPC_FLAG_GRAVITY, false)
    Call(UseSettingsFrom, CAM_DEFAULT, -1105, -171, 225)
    Call(SetPanTarget, CAM_DEFAULT, -1105, -171, 225)
    Call(SetCamDistance, CAM_DEFAULT, Float(300.0))
    Call(SetCamSpeed, CAM_DEFAULT, Float(90.0))
    Call(PanToTarget, CAM_DEFAULT, 0, true)
    Call(WaitForCam, CAM_DEFAULT, Float(1.0))
    Set(LVar2, 360)
    Call(MakeLerp, -400, -171, 60, EASING_QUADRATIC_OUT)
    Loop(0)
        Call(UpdateLerp)
        Call(SetPlayerPos, -1105, LVar0, 225)
        Call(SetNpcPos, NPC_PARTNER, -1105, LVar0, 215)
        Add(LVar2, 40)
        Call(InterpPlayerYaw, LVar2, 0)
        Call(SetNpcRotation, NPC_PARTNER, 0, LVar2, 0)
        Wait(1)
        IfEq(LVar1, 0)
            BreakLoop
        EndIf
    EndLoop
    Set(LVar0, LVar2)
    Sub(LVar0, 360)
    Sub(LVar2, LVar0)
    Set(LVar0, 40)
    Label(10)
        IfLt(LVar2, 450)
            Call(InterpPlayerYaw, LVar2, 0)
            Call(SetNpcRotation, NPC_PARTNER, 0, LVar2, 0)
            Add(LVar2, LVar0)
            Wait(1)
            Goto(10)
        EndIf
        Set(LVar2, 90)
        Sub(LVar0, 5)
        IfGt(LVar0, 20)
            Goto(10)
        EndIf
    Call(SetNpcRotation, NPC_PARTNER, 0, 0, 0)
    Call(N(func_80240EF4_A17114), MV_StarBeamEffect)
    Call(SetNpcFlagBits, NPC_PARTNER, NPC_FLAG_GRAVITY, true)
    Call(EnablePartnerAI)
    Call(DisablePlayerPhysics, false)
    Call(PanToTarget, CAM_DEFAULT, 0, false)
    Call(SetMusic, 0, SONG_STAR_WAY_OPENS, BGM_VARIATION_3, VOL_LEVEL_FULL)
    Call(DisablePlayerInput, false)
    Call(AwaitPlayerLeave, -1105, 225, 40)
    BindTrigger(Ref(N(EVS_ExitStarBeam)), TRIGGER_FLOOR_TOUCH, COLLIDER_deiliw, 1, 0)
    Return
    End
};

EvtScript N(EVS_Main) = {
    Set(GB_WorldLocation, LOCATION_STAR_WAY)
    Call(SetSpriteShading, SHADING_NONE)
    EVT_SETUP_CAMERA_DEFAULT()
    Set(GF_MAP_StarWay, true)
    Call(MakeNpcs, false, Ref(N(DefaultNPCs)))
    Call(GetEntryID, LVar0)
    Switch(LVar0)
        CaseEq(hos_02_ENTRY_0)
            Thread
                ExecWait(N(EVS_EnterStarBeam))
                Exec(N(EVS_BindExitTriggers))
            EndThread
        CaseEq(hos_02_ENTRY_1)
            Call(ModifyColliderFlags, MODIFY_COLLIDER_FLAGS_SET_BITS, COLLIDER_deilite, COLLIDER_FLAGS_UPPER_MASK)
            Set(LVar0, Ref(N(EVS_BindExitTriggers)))
            Exec(EnterWalk)
    EndSwitch
    Exec(N(EVS_SetupModelFX))
    Exec(N(EVS_SetupUnused))
    Exec(N(EVS_SetupMusic))
    Return
    End
};
