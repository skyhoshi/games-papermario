#include "omo_09.h"

s32 N(ConveyorColliders)[4] = {
    COLLIDER_o904,
    COLLIDER_o906,
    COLLIDER_o907,
    COLLIDER_o911
};

//TODO Vec2f[4]
f32 N(ConveyorPushVels)[4][2] = {
    { -3.9,  0.0 },
    {  3.9,  0.0 },
    {  0.0,  3.9 },
    {  0.0, -3.9 },
};

s32 N(ShouldPauseConveyor)(void) {
    PlayerStatus* playerStatus = &gPlayerStatus;
    PlayerData* playerData = &gPlayerData;

    if (playerStatus->flags & PS_FLAG_PAUSED) {
        return true;
    }

    if (gPartnerStatus.partnerActionState != PARTNER_ACTION_NONE &&
        (playerData->curPartner == PARTNER_GOOMBARIO || playerData->curPartner == PARTNER_SUSHIE))
    {
        return true;
    }

    return false;
}

API_CALLABLE(N(WaitWhileConveyorPaused)) {
    if (!N(ShouldPauseConveyor)()) {
        return ApiStatus_DONE2;
    }
    return ApiStatus_BLOCK;
}

API_CALLABLE(N(AddConveyorPush)) {
    PlayerStatus* playerStatus = &gPlayerStatus;
    Npc* partner = get_npc_unsafe(NPC_PARTNER);
    PartnerStatus* partnerStatus = &gPartnerStatus;
    f32 x, y, z;
    f32 outLength;
    f32 hitRx, hitRz;
    f32 hitDirX, hitDirZ;
    s32 hit;
    s32 one;
    u32 i;

    script->varTable[0] = 0;
    if (N(ShouldPauseConveyor)()) {
        return ApiStatus_DONE2;
    }

    one = 1;
    if (partnerStatus->actingPartner == PARTNER_BOW) {
        if (partnerStatus->partnerActionState != PARTNER_ACTION_NONE && playerStatus->curAlpha == 128) {
            x = playerStatus->pos.x;
            y = playerStatus->pos.y;
            z = playerStatus->pos.z;
            outLength = 1000.0f;
            hit = player_raycast_below_cam_relative(playerStatus,
                &x, &y, &z, &outLength,
                &hitRx, &hitRz, &hitDirX, &hitDirZ
            );

            for (i = 0; i < ARRAY_COUNT(N(ConveyorColliders)); i++) {
                if (hit == N(ConveyorColliders)[i]) {
                    playerStatus->pushVel.x = N(ConveyorPushVels)[i][0];
                    playerStatus->pushVel.z = N(ConveyorPushVels)[i][one]; // TODO needed to match
                }
            }
            script->varTable[0] = 1;
            return ApiStatus_DONE2;
        }
    }
    one = 1;
    if (partnerStatus->actingPartner != PARTNER_LAKILESTER ||
        partnerStatus->partnerActionState == PARTNER_ACTION_NONE)
    {
        for (i = 0; i < ARRAY_COUNT(N(ConveyorColliders)); i++) {
            if (gCollisionStatus.curFloor == N(ConveyorColliders)[i] ||
                gCollisionStatus.lastTouchedFloor == N(ConveyorColliders)[i])
            {
                playerStatus->pushVel.x = N(ConveyorPushVels)[i][0];
                playerStatus->pushVel.z = N(ConveyorPushVels)[i][one]; // TODO needed to match
            }

            if (partner->curFloor == N(ConveyorColliders)[i] &&
                ((partnerStatus->actingPartner != PARTNER_KOOPER) ||
                 (partnerStatus->partnerActionState == PARTNER_ACTION_NONE)))
            {
                partner->pos.x += N(ConveyorPushVels)[i][0];
                partner->pos.z += N(ConveyorPushVels)[i][one]; // TODO needed to match
                partner_clear_player_tracking(partner);
            }
        }
    }

    return ApiStatus_DONE2;
}

EvtScript N(EVS_SetupConveyors) = {
    SetGroup(EVT_GROUP_HOSTILE_NPC)
    Call(EnableTexPanning, MODEL_1, true)
    Call(EnableTexPanning, MODEL_3, true)
    Call(EnableTexPanning, MODEL_4, true)
    Call(EnableTexPanning, MODEL_8, true)
    Thread
        Set(LVar0, 0)
        Label(0)
            Call(N(WaitWhileConveyorPaused))
            Add(LVar1, -1280)
            Call(SetTexPanOffset, TEX_PANNER_1, TEX_PANNER_MAIN, 0, LVar1)
            Call(SetTexPanOffset, TEX_PANNER_3, TEX_PANNER_MAIN, 0, LVar1)
            Call(SetTexPanOffset, TEX_PANNER_4, TEX_PANNER_MAIN, 0, LVar1)
            Call(SetTexPanOffset, TEX_PANNER_8, TEX_PANNER_MAIN, 0, LVar1)
            Wait(1)
            Goto(0)
    EndThread
    Thread
        Label(10)
            Call(N(AddConveyorPush))
            Wait(1)
            Goto(10)
    EndThread
    Return
    End
};
