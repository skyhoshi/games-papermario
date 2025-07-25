#include "nok_12.h"

#include "world/common/enemy/KoopaTroopa_Wander.inc.c"
#include "world/common/enemy/KoopaTroopa_Patrol.inc.c"
#include "world/common/enemy/Goomba_Wander.inc.c"
#include "world/common/enemy/SpikedGoomba_Wander.inc.c"
#include "world/common/enemy/SpikedGoomba_Patrol.inc.c"

// 'sleep' on top of brick block until it's broken
EvtScript N(EVS_NpcIdle_SpikedGoomba) = {
    Label(0)
    Call(GetSelfVar, 0, LVar0)
    IfEq(LVar0, 0)
        Wait(1)
        Goto(0)
    EndIf
    Call(SetNpcAnimation, NPC_SELF, ANIM_SpikedGoomba_Sleep)
    Thread
        Call(PlaySoundAtNpc, NPC_SELF, SOUND_AI_FOUND_PLAYER_JUMP, SOUND_SPACE_DEFAULT)
        Call(MakeLerp, -90, 0, 10, EASING_LINEAR)
        Label(1)
        Call(UpdateLerp)
        Call(SetNpcRotation, NPC_SELF, LVar0, 0, 0)
        Wait(1)
        IfEq(LVar1, 1)
            Goto(1)
        EndIf
        Call(GetNpcPos, NPC_SELF, LVarA, LVarB, LVarC)
        Call(MakeLerp, 0, 360, 15, EASING_LINEAR)
        Label(2)
        Call(UpdateLerp)
        Call(SetNpcRotation, NPC_SELF, 0, LVar0, 0)
        Wait(1)
        IfEq(LVar1, 1)
            Goto(2)
        EndIf
        Call(SetNpcRotation, NPC_SELF, 0, 0, 0)
    EndThread
    Call(SetNpcJumpscale, NPC_SELF, Float(0.7))
    Call(NpcJump0, NPC_SELF, -65, 0, -120, 25)
    Call(SetNpcAnimation, NPC_SELF, ANIM_SpikedGoomba_Hurt)
    Wait(20)
    Call(SetNpcAnimation, NPC_SELF, ANIM_SpikedGoomba_Idle)
    Call(BindNpcAI, NPC_SELF, Ref(N(EVS_NpcAI_SpikedGoomba_Wander)))
    Return
    End
};

EvtScript N(EVS_NpcInit_SpikedGoomba) = {
    Call(SetNpcPos, NPC_SELF, -165, 86, -118)
    Call(SetNpcRotation, NPC_SELF, -85, 0, 0)
    Call(BindNpcIdle, NPC_SELF, Ref(N(EVS_NpcIdle_SpikedGoomba)))
    Return
    End
};

EvtScript N(EVS_NpcIdle_KoopaTroopa_02) = {
    Return
    End
};

EvtScript N(EVS_KoopaTroopa_Demo_MissAttack) = {
    Wait(45)
    Call(SetNpcAnimation, NPC_SELF, ANIM_KoopaTroopa_ShellEnter)
    Wait(8)
    Call(SetNpcAnimation, NPC_SELF, ANIM_KoopaTroopa_ShellSpin)
    Wait(6)
    Call(GetNpcPos, NPC_SELF, LVar0, LVar1, LVar2)
    Sub(LVar0, 80)
    Add(LVar2, 100)
    Call(NpcMoveTo, NPC_SELF, LVar0, LVar2, 12)
    Call(SetNpcAnimation, NPC_SELF, ANIM_KoopaTroopa_ShellExit)
    Wait(8)
    Call(SetNpcAnimation, NPC_SELF, ANIM_KoopaTroopa_Idle)
    Return
    End
};

EvtScript N(EVS_NpcInit_KoopaTroopa_02) = {
    Call(GetEntryID, LVar0)
    IfGe(LVar0, nok_12_ENTRY_2)
        Call(BindNpcIdle, NPC_SELF, Ref(N(EVS_NpcIdle_KoopaTroopa_02)))
        Call(SetNpcPos, NPC_SELF, 310, 0, -165)
    EndIf
    Return
    End
};

EvtScript N(EVS_NpcInit_KoopaTroopa_02_Demo) = {
    Call(GetEntryID, LVar0)
    IfGe(LVar0, nok_12_ENTRY_2)
        Call(BindNpcIdle, NPC_SELF, Ref(N(EVS_KoopaTroopa_Demo_MissAttack)))
    EndIf
    Return
    End
};

NpcData N(NpcData_KoopaTroopa_01) = {
    .id = NPC_KoopaTroopa_01,
    .pos = { -370.0f, 0.0f, -25.0f },
    .yaw = 270,
    .territory = {
        .patrol = {
            .isFlying = true,
            .moveSpeedOverride = NO_OVERRIDE_MOVEMENT_SPEED,
            .numPoints  = 2,
            .points  = {
                { -370, 0, -25 },
                { -455, 0, -25 },
            },
            .detectShape = SHAPE_CYLINDER,
            .detectPos  = { -436, 0, -104 },
            .detectSize = { 200 },
        }
    },
    .settings = &N(NpcSettings_KoopaTroopa_Patrol),
    .flags = ENEMY_FLAG_IGNORE_ENTITY_COLLISION | ENEMY_FLAG_FLYING,
    .drops = KOOPA_TROOPA_NOK_DROPS,
    .animations = KOOPA_TROOPA_ANIMS,
    .aiDetectFlags = AI_DETECT_SIGHT,
};

NpcData N(NpcData_KoopaTroopa_02) = {
    .id = NPC_KoopaTroopa_02,
    .pos = { 563.0f, 50.0f, -43.0f },
    .yaw = 270,
    .territory = {
        .wander = {
            .isFlying = false,
            .moveSpeedOverride = NO_OVERRIDE_MOVEMENT_SPEED,
            .wanderShape = SHAPE_CYLINDER,
            .centerPos  = { 563, 50, -43 },
            .wanderSize = { 50 },
            .detectShape = SHAPE_CYLINDER,
            .detectPos  = { 563, 50, -43 },
            .detectSize = { 500 },
        }
    },
    .init = &N(EVS_NpcInit_KoopaTroopa_02),
    .settings = &N(NpcSettings_KoopaTroopa_Wander),
    .flags = ENEMY_FLAG_IGNORE_ENTITY_COLLISION | ENEMY_FLAG_FLYING,
    .drops = KOOPA_TROOPA_NOK_DROPS,
    .animations = KOOPA_TROOPA_ANIMS,
    .aiDetectFlags = AI_DETECT_SIGHT,
};

NpcData N(NpcData_KoopaTroopa_02_Demo) = {
    .id = NPC_KoopaTroopa_02,
    .pos = { 600.0f, 50.0f, -75.0f },
    .yaw = 270,
    .territory = {
        .wander = {
            .isFlying = false,
            .moveSpeedOverride = NO_OVERRIDE_MOVEMENT_SPEED,
            .wanderShape = SHAPE_CYLINDER,
            .centerPos  = { 563, 50, -43 },
            .wanderSize = { 50 },
            .detectShape = SHAPE_CYLINDER,
            .detectPos  = { 563, 50, -43 },
            .detectSize = { 500 },
        }
    },
    .init = &N(EVS_NpcInit_KoopaTroopa_02_Demo),
    .settings = &N(NpcSettings_KoopaTroopa_Wander),
    .flags = ENEMY_FLAG_IGNORE_ENTITY_COLLISION | ENEMY_FLAG_FLYING,
    .drops = KOOPA_TROOPA_NOK_DROPS,
    .animations = KOOPA_TROOPA_ANIMS,
    .aiDetectFlags = AI_DETECT_SIGHT,
};

NpcData N(NpcData_Goomba) = {
    .id = NPC_Goomba,
    .pos = { 50.0f, 0.0f, -72.0f },
    .yaw = 270,
    .territory = {
        .wander = {
            .isFlying = true,
            .moveSpeedOverride = NO_OVERRIDE_MOVEMENT_SPEED,
            .wanderShape = SHAPE_CYLINDER,
            .centerPos  = { 50, 0, -72 },
            .wanderSize = { 50 },
            .detectShape = SHAPE_RECT,
            .detectPos  = { 50, 0, -72 },
            .detectSize = { 150, 80 },
        }
    },
    .settings = &N(NpcSettings_Goomba_Wander),
    .flags = ENEMY_FLAG_IGNORE_ENTITY_COLLISION | ENEMY_FLAG_FLYING,
    .drops = GOOMBA_DROPS,
    .animations = GOOMBA_ANIMS,
    .aiDetectFlags = AI_DETECT_SIGHT,
};

NpcData N(NpcData_SpikedGoomba) = {
    .id = NPC_SpikedGoomba,
    .pos = { -160.0f, 0.0f, -120.0f },
    .yaw = 270,
    .territory = {
        .wander = {
            .isFlying = true,
            .moveSpeedOverride = NO_OVERRIDE_MOVEMENT_SPEED,
            .wanderShape = SHAPE_CYLINDER,
            .centerPos  = { -84, 0, -72 },
            .wanderSize = { 50 },
            .detectShape = SHAPE_RECT,
            .detectPos  = { -84, 0, -72 },
            .detectSize = { 150, 80 },
        }
    },
    .init = &N(EVS_NpcInit_SpikedGoomba),
    .settings = &N(NpcSettings_SpikedGoomba_Wander),
    .flags = ENEMY_FLAG_IGNORE_ENTITY_COLLISION | ENEMY_FLAG_FLYING,
    .drops = SPIKED_GOOMBA_DROPS,
    .animations = SPIKED_GOOMBA_ANIMS,
    .aiDetectFlags = AI_DETECT_SIGHT,
};

NpcGroupList N(DefaultNPCs) = {
    NPC_GROUP(N(NpcData_KoopaTroopa_01), BTL_NOK_FORMATION_09, BTL_NOK_STAGE_00),
    NPC_GROUP(N(NpcData_KoopaTroopa_02), BTL_NOK_FORMATION_0F, BTL_NOK_STAGE_00),
    NPC_GROUP(N(NpcData_Goomba), BTL_NOK_FORMATION_01, BTL_NOK_STAGE_01),
    NPC_GROUP(N(NpcData_SpikedGoomba), BTL_NOK_FORMATION_04, BTL_NOK_STAGE_01),
    {}
};

NpcGroupList N(DemoNPCs) = {
    NPC_GROUP(N(NpcData_KoopaTroopa_01), BTL_NOK_FORMATION_09, BTL_NOK_STAGE_00),
    NPC_GROUP(N(NpcData_KoopaTroopa_02_Demo), BTL_NOK_FORMATION_0F, BTL_NOK_STAGE_00),
    NPC_GROUP(N(NpcData_Goomba), BTL_NOK_FORMATION_01, BTL_NOK_STAGE_01),
    NPC_GROUP(N(NpcData_SpikedGoomba), BTL_NOK_FORMATION_04, BTL_NOK_STAGE_01),
    {}
};
