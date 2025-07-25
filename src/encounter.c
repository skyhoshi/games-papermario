#include "battle/battle.h"
#include "script_api/battle.h"
#include "audio.h"
#include "audio/public.h"
#include "npc.h"
#include "effects.h"
#include "hud_element.h"
#include "world/partners.h"
#include "sprite.h"
#include "sprite/npc/BattleMerlee.h"
#include "sprite/player.h"
#include "model.h"

API_CALLABLE(ShowMerleeCoinMessage);
API_CALLABLE(ShowMerleeRanOutMessage);
API_CALLABLE(FadeInMerlee);
API_CALLABLE(FadeOutMerlee);
API_CALLABLE(MerleeUpdateFX);
API_CALLABLE(MerleeStopFX);
API_CALLABLE(PlayMerleeGatherFX);
API_CALLABLE(PlayMerleeOrbFX);

bool D_80077C40 = false;

bool EncounterStateChanged;

EvtScript EVS_MerleeDropCoins = {
    Wait(10)
    Call(FadeBackgroundDarken)
    Wait(10)
    Call(CreateNpc, NPC_BTL_MERLEE, ANIM_BattleMerlee_Gather)
    Call(SetNpcFlagBits, NPC_BTL_MERLEE, NPC_FLAG_IGNORE_PLAYER_COLLISION, true)
    Call(SetNpcYaw, NPC_BTL_MERLEE, 0)
    Call(GetCamLookAtObjVector)
    Call(SetNpcPos, NPC_BTL_MERLEE, LVar0, LVar1, LVar2)
    Thread
        Call(MerleeUpdateFX)
    EndThread
    Call(FadeInMerlee)
    Wait(30)
    Call(SetNpcAnimation, NPC_BTL_MERLEE, ANIM_BattleMerlee_Release)
    Call(MerleeStopFX)
    Call(FadeBackgroundLighten)
    Wait(20)
    Thread
        Call(FadeOutMerlee)
        Call(DeleteNpc, NPC_BTL_MERLEE)
    EndThread
    Call(PlaySound, SOUND_MAGIC_DESCENDING)
    Call(GetPlayerPos, LVar0, LVar1, LVar2)
    Call(PlayMerleeGatherFX, LVar0, LVar1, LVar2)
    Call(PlayMerleeOrbFX, LVar0, LVar1, LVar2)
    Wait(15)
    Call(ShowMerleeCoinMessage)
    Wait(15)
    Call(HasMerleeCasts)
    IfEq(LVar0, 1)
        Return
    EndIf
    Call(ShowMerleeRanOutMessage)
    Wait(15)
    Return
    End
};

EvtScript EVS_NpcDefeat = {
    Call(GetBattleOutcome, LVar0)
    Switch(LVar0)
        CaseEq(OUTCOME_PLAYER_WON)
            Call(OnDefeatEnemy)
        CaseEq(OUTCOME_PLAYER_LOST)
        CaseEq(OUTCOME_PLAYER_FLED)
    EndSwitch
    Return
    End
};

EvtScript EVS_FleeBattleDrops = {
    Call(OnFleeBattleDrops)
    Return
    End
};

EnemyDrops DefaultEnemyDrops = {
    .dropFlags = NPC_DROP_FLAG_80,
    .itemDropChance = 10,
    .itemDrops = {
        {
            .item = ITEM_MUSHROOM,
            .weight = 50,
            .flagIdx = -1,
        },
    },
    .heartDrops = {
        {
            .cutoff = F16(75),
            .generalChance = F16(100),
            .attempts = 0,
            .chancePerAttempt = 1,
        },
        {
            .cutoff = F16(50),
            .generalChance = F16(75),
            .attempts = 0,
            .chancePerAttempt = 2,
        },
        {
            .cutoff = F16(25),
            .generalChance = F16(50),
            .attempts = 0,
            .chancePerAttempt = 3,
        },
        {
            .cutoff = F16(0),
            .generalChance = F16(25),
            .attempts = 0,
            .chancePerAttempt = 4,
        },
    },
    .flowerDrops = {
        {
            .cutoff = 1,
            .generalChance = 3,
            .attempts = 0,
            .chancePerAttempt = 0,
        },
    },
    .minCoinBonus = 0,
    .maxCoinBonus = 0,
};

EvtScript EnemyNpcHit = {
    Call(GetOwnerEncounterTrigger, LVar0)
    Switch(LVar0)
        CaseEq(ENCOUNTER_TRIGGER_NONE)
        CaseOrEq(ENCOUNTER_TRIGGER_JUMP)
        CaseOrEq(ENCOUNTER_TRIGGER_HAMMER)
        CaseOrEq(ENCOUNTER_TRIGGER_PARTNER)
            Call(GetSelfAnimationFromTable, ENEMY_ANIM_INDEX_HIT, LVar0)
            ExecWait(EVS_NpcHitRecoil)
        CaseEq(ENCOUNTER_TRIGGER_SPIN)
            Thread
                Call(func_800458CC, LVar0)
                IfEq(LVar0, 0)
                    Set(LVarA, 0)
                    Loop(30)
                        Add(LVarA, 40)
                        Call(SetNpcRotation, NPC_SELF, 0, LVarA, 0)
                        Wait(1)
                    EndLoop
                EndIf
            EndThread
        EndCaseGroup
    EndSwitch
    Return
    End
};

EvtScript EnemyNpcDefeat = {
    Call(SetNpcRotation, NPC_SELF, 0, 0, 0)
    Call(GetBattleOutcome, LVar0)
    Switch(LVar0)
        CaseEq(OUTCOME_PLAYER_WON)
            Call(DoNpcDefeat)
        CaseEq(OUTCOME_PLAYER_FLED)
            Call(OnPlayerFled, 0)
        CaseEq(OUTCOME_ENEMY_FLED)
            Call(SetEnemyFlagBits, NPC_SELF, ENEMY_FLAG_FLED, true)
            Call(RemoveNpc, NPC_SELF)
    EndSwitch
    Return
    End
};

s32 gEncounterState;
s32 gEncounterSubState;
EncounterStatus gCurrentEncounter;

s8 HasPreBattleSongPushed;
s8 PendingPartnerAbilityResume;
s8 LastBattleStartedBySpin;
s16 gFirstStrikeMessagePos;

BSS s32 WorldMerleeEffectsTime;
BSS f32 WorldMerleeBasePosY;
BSS EffectInstance* WorldMerleeOrbEffect;
BSS EffectInstance* WorldMerleeWaveEffect;
BSS Evt* MerleeDropCoinsEvt;
BSS s32 MerleeDropCoinsEvtID;
BSS s16 WorldMerleeEffectsState;

enum {
    MERLEE_EFFECTS_HOLD     = 0, // effects appear and track Merlee's position
    MERLEE_EFFECTS_RELEASE  = 1, // effects grow larger before vanishing
    MERLEE_EFFECTS_DISMISS  = 2, // effects vanish and are dismissed
};

void set_battle_formation(Battle*);
void setup_status_bar_for_world(void);
void partner_handle_after_battle(void);
s32 get_coin_drop_amount(Enemy* enemy);

s32 get_defeated(s32 mapID, s32 encounterID) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    s32 encounterIdx = encounterID / 32;
    s32 encounterShift = encounterID % 32;

    return currentEncounter->defeatFlags[mapID][encounterIdx] & (1 << encounterShift);
}

void set_defeated(s32 mapID, s32 encounterID) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    s32 encounterIdx = encounterID / 32;
    s32 encounterShift;
    s32 flag;

    flag = encounterID % 32;
    encounterShift = flag;
    flag = currentEncounter->defeatFlags[mapID][encounterIdx];
    currentEncounter->defeatFlags[mapID][encounterIdx] = flag | (1 << encounterShift);

    // TODO: The below should work but has regalloc issues:
    /*EncounterStatus *currentEncounter = &gCurrentEncounter;
    s32 encounterIdx = encounterID / 32;
    s32 encounterShift = encounterID % 32;

    currentEncounter->defeatFlags[mapID][encounterIdx] |= (1 << encounterShift);*/
}

API_CALLABLE(ShowMerleeCoinMessage) {
    if (isInitialCall) {
        show_merlee_message(0, 60);
    }

    if (is_merlee_message_done()) {
        return ApiStatus_BLOCK;
    } else {
        return ApiStatus_DONE2;
    }
}

API_CALLABLE(ShowMerleeRanOutMessage) {
    if (isInitialCall) {
        show_merlee_message(1, 60);
    }

    if (is_merlee_message_done()) {
        return ApiStatus_BLOCK;
    } else {
        return ApiStatus_DONE2;
    }
}

API_CALLABLE(FadeBackgroundDarken) {
    if (isInitialCall) {
        mdl_set_all_tint_type(ENV_TINT_SHROUD);
        *gBackgroundTintModePtr = ENV_TINT_SHROUD;
        mdl_set_shroud_tint_params(0, 0, 0, 0);
        script->functionTemp[0] = 25;
    }

    mdl_set_shroud_tint_params(0, 0, 0, ((25 - script->functionTemp[0]) * 10) & 254);
    script->functionTemp[0]--;

    if (script->functionTemp[0] == 0) {
        return ApiStatus_DONE2;
    } else {
        return ApiStatus_BLOCK;
    }
}

API_CALLABLE(FadeBackgroundLighten) {
    if (isInitialCall) {
        script->functionTemp[0] = 25;
    }

    mdl_set_shroud_tint_params(0, 0, 0, (script->functionTemp[0] * 10) & 254);
    script->functionTemp[0] -= 5;

    if (script->functionTemp[0] == 0) {
        mdl_set_shroud_tint_params(0, 0, 0, 0);
        return ApiStatus_DONE2;
    } else {
        return ApiStatus_BLOCK;
    }
}

API_CALLABLE(FadeInMerlee) {
    Npc* npc = get_npc_unsafe(NPC_BTL_MERLEE);

    if (isInitialCall) {
        sfx_play_sound(SOUND_MERLEE_APPEAR);
        npc->alpha = 0;
    }

    npc->alpha += 17;

    if ((u32)(npc->alpha & 0xFF) >= 0xFF) {
        npc->alpha = 0xFF;
        return ApiStatus_DONE1;
    } else {
        return ApiStatus_BLOCK;
    }
}

API_CALLABLE(FadeOutMerlee) {
    Npc* npc = get_npc_unsafe(NPC_BTL_MERLEE);

    npc->alpha -= 17;
    if (npc->alpha == 0) {
        npc->alpha = 0;
        return ApiStatus_DONE1;
    } else {
        return ApiStatus_BLOCK;
    }
}

// same as BattleMerleeUpdateFX aside from syms
API_CALLABLE(MerleeUpdateFX) {
    Npc* merlee = get_npc_unsafe(NPC_BTL_MERLEE);
    EnergyOrbWaveFXData* effectData;

    if (isInitialCall) {
        script->functionTemp[1] = 0;
        WorldMerleeBasePosY = merlee->pos.y;
        WorldMerleeOrbEffect = fx_energy_orb_wave(0, merlee->pos.x, merlee->pos.y, merlee->pos.z, 0.4f, 0);
        WorldMerleeWaveEffect = fx_energy_orb_wave(3, merlee->pos.x, merlee->pos.y, merlee->pos.z, 0.00001f, 0);
        WorldMerleeEffectsState = MERLEE_EFFECTS_HOLD;
        WorldMerleeEffectsTime = 12;
        sfx_play_sound(SOUND_MAGIC_ASCENDING);
    }

    merlee->pos.y = WorldMerleeBasePosY + sin_rad(DEG_TO_RAD(script->functionTemp[1])) * 3.0f;

    script->functionTemp[1] += 10;
    script->functionTemp[1] = clamp_angle(script->functionTemp[1]);

    effectData = WorldMerleeOrbEffect->data.energyOrbWave;
    effectData->pos.x = merlee->pos.x;
    effectData->pos.y = merlee->pos.y + 16.0f;
    effectData->pos.z = merlee->pos.z;

    effectData = WorldMerleeWaveEffect->data.energyOrbWave;
    effectData->pos.x = merlee->pos.x;
    effectData->pos.y = merlee->pos.y + 16.0f;
    effectData->pos.z = merlee->pos.z + 5.0f;

    if (WorldMerleeEffectsState == MERLEE_EFFECTS_DISMISS) {
        WorldMerleeOrbEffect->data.energyOrbWave->scale = 0.00001f;
        WorldMerleeWaveEffect->data.energyOrbWave->scale = 0.00001f;
        WorldMerleeOrbEffect->flags |= FX_INSTANCE_FLAG_DISMISS;
        WorldMerleeWaveEffect->flags |= FX_INSTANCE_FLAG_DISMISS;
        return ApiStatus_DONE1;
    }

    if (WorldMerleeEffectsState == MERLEE_EFFECTS_RELEASE) {
        effectData = WorldMerleeOrbEffect->data.energyOrbWave;
        effectData->scale += 0.35;
        if (effectData->scale > 3.5) {
            effectData->scale = 3.5;
        }

        if (WorldMerleeEffectsTime != 0) {
            WorldMerleeEffectsTime--;
        } else {
            effectData = WorldMerleeWaveEffect->data.energyOrbWave;
            effectData->scale += 0.5;
            if (effectData->scale > 5.0) {
                WorldMerleeEffectsState = MERLEE_EFFECTS_DISMISS;
            }
        }
    }
    return ApiStatus_BLOCK;
}

API_CALLABLE(MerleeStopFX) {
    WorldMerleeEffectsState = MERLEE_EFFECTS_RELEASE;
    return ApiStatus_DONE2;
}

API_CALLABLE(GetCamLookAtObjVector) {
    script->varTable[0] = gCameras[gCurrentCameraID].lookAt_obj.x;
    script->varTable[1] = gCameras[gCurrentCameraID].lookAt_obj.y;
    script->varTable[2] = gCameras[gCurrentCameraID].lookAt_obj.z;

    return ApiStatus_DONE2;
}

API_CALLABLE(HasMerleeCasts) {
    script->varTable[0] = false;
    if (gPlayerData.merleeCastsLeft > 0) {
        script->varTable[0] = true;
    }

    return ApiStatus_DONE2;
}

API_CALLABLE(PlayMerleeGatherFX) {
    Bytecode* args = script->ptrReadPos;
    s32 var0 = evt_get_variable(script, *args++);
    s32 var1 = evt_get_variable(script, *args++);
    s32 var2 = evt_get_variable(script, *args++);

    fx_energy_in_out(6, var0, var1, var2, 1.2f, 30);
    return ApiStatus_DONE2;
}

API_CALLABLE(PlayMerleeOrbFX) {
    Bytecode* args = script->ptrReadPos;
    s32 var0 = evt_get_variable(script, *args++);
    s32 var1 = evt_get_variable(script, *args++);
    s32 var2 = evt_get_variable(script, *args++);

    fx_energy_orb_wave(9, var0, var1, var2, 5.0f, 15);
    return ApiStatus_DONE2;
}

API_CALLABLE(OnDefeatEnemy) {
    Enemy* enemy = script->owner1.enemy;
    Npc* npc = get_npc_unsafe(enemy->npcID);
    s32 temp1;

    if (isInitialCall) {
        script->functionTemp[0] = 0;
        script->functionTemp[1] = 20;
    }

    if (script->functionTemp[1] & 1) {
        npc->flags &= ~NPC_FLAG_INVISIBLE;
    } else {
        npc->flags |= NPC_FLAG_INVISIBLE;
    }

    if (script->functionTemp[1] == 15) {
        sfx_play_sound(SOUND_ACTOR_DEATH);
        fx_damage_stars(FX_DAMAGE_STARS_1, npc->pos.x, npc->pos.y + (npc->collisionHeight / 2), npc->pos.z, 0, -1.0f, 0, 10);
    }

    temp1 = script->functionTemp[1];
    if (script->functionTemp[1] == 10) {
        fx_big_smoke_puff(npc->pos.x, npc->pos.y + 10.0f, npc->pos.z + 10.0f);
        if (script->functionTemp[1] == temp1) { // what? (never can be false, seemingly)
            spawn_drops(enemy);
        }
    }

    script->functionTemp[1]--;
    if (script->functionTemp[1] == 0) {
        npc->flags |= NPC_FLAG_INVISIBLE;
        return ApiStatus_DONE1;
    }

    return ApiStatus_BLOCK;
}

API_CALLABLE(OnFleeBattleDrops) {
    PlayerStatus* playerStatus = &gPlayerStatus;
    PlayerData* playerData = &gPlayerData;

    if (isInitialCall) {
        script->functionTemp[0] = 0;
        script->functionTemp[1] = 40;
        script->functionTemp[2] = 0;
    }

    script->functionTemp[2]++;
    if (script->functionTemp[2] >= 5) {
        if (rand_int(100) < 50) {
            if (playerData->coins != 0) {
                playerData->coins--;
                make_item_entity_delayed(ITEM_COIN, playerStatus->pos.x,
                    playerStatus->pos.y + playerStatus->colliderHeight, playerStatus->pos.z,
                    ITEM_SPAWN_MODE_TOSS_SPAWN_ALWAYS, 0, 0);
            }
        }
        script->functionTemp[2] = 0;
    }

    return --script->functionTemp[1] == 0;
}

void update_encounters_neutral(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    PlayerStatus* playerStatus = &gPlayerStatus;
    Camera* camera = &gCameras[gCurrentCameraID];
    s32 screenX, screenY, screenZ;
    f32 npcX, npcY, npcZ;
    f32 npcYaw;
    f32 testX, testY, testZ;
    f32 x, y, z;
    s32 e;
    f32 playerX, playerY, playerZ;
    f32 playerYaw;
    Encounter* encounter;
    Evt* script;
    Npc* npc;
    f32 distance;
    f32 colHeight;
    f32 colRadius;
    f32 hammerDir;

    s32 triggeredBattle;
    s32 cond2;
    s32 firstStrikeType;
    s32 suspendTime;

    Enemy* enemy;
    Enemy* currentEnemy;
    s32 i;

    f32 playerJumpColHeight = 37.0f;
    f32 playerColRadius = 14.0f;
    f32 playerColHeight = 18.0f;

    f32 dx, dz;
    f32 angle1, angle2;

    do {
        if (currentEncounter->hitType == ENCOUNTER_TRIGGER_CONVERSATION) {
            goto START_BATTLE;
        }
    } while (0);

    currentEncounter->songID = AU_SONG_NONE;
    currentEncounter->unk_18 = -1;
    currentEncounter->hitType = 0;
    currentEncounter->forbidFleeing = false;
    currentEncounter->dropWhackaBump = false;
    currentEncounter->flags &= ~ENCOUNTER_FLAG_THUMBS_UP;
    currentEncounter->flags &= ~ENCOUNTER_FLAG_CANT_SKIP_WIN_DELAY;
    currentEncounter->flags &= ~ENCOUNTER_FLAG_SKIP_FLEE_DROPS;

    playerX = playerStatus->pos.x;
    playerY = playerStatus->pos.y;
    playerZ = playerStatus->pos.z;
    playerYaw = playerStatus->spriteFacingAngle;

    if (playerYaw < 180.0f) {
        playerYaw = clamp_angle(camera->curYaw - 90.0f);
    } else {
        playerYaw = clamp_angle(camera->curYaw + 90.0f);
    }

    if (currentEncounter->battleTriggerCooldown != 0) {
        if (!(gOverrideFlags & GLOBAL_OVERRIDES_40)) {
            currentEncounter->battleTriggerCooldown--;
        }
        if (playerStatus->blinkTimer != 0) {
            if (!(playerStatus->flags & PS_FLAG_INPUT_DISABLED)) {
                playerStatus->blinkTimer = currentEncounter->battleTriggerCooldown;
            } else {
                playerStatus->blinkTimer = 1;
            }
        }
    }

    for (e = 0; e < currentEncounter->numEncounters; e++) {
        encounter = currentEncounter->encounterList[e];
        if (encounter == NULL) {
            continue;
        }
        for (i = 0; i < encounter->count; i++) {
            enemy = encounter->enemy[i];
            if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                continue;
            }
            npc = get_npc_unsafe(enemy->npcID);
            if (enemy->aiSuspendTime != 0) {
                if (!(gOverrideFlags & GLOBAL_OVERRIDES_40)) {
                    enemy->aiSuspendTime--;
                    suspendTime = enemy->aiSuspendTime;
                } else {
                    suspendTime = 0;
                }

                if (suspendTime & 1) {
                    npc->flags |= NPC_FLAG_SUSPENDED;
                    enemy->flags |= ENEMY_FLAG_SUSPENDED;
                } else {
                    npc->flags &= ~NPC_FLAG_SUSPENDED;
                    enemy->flags &= ~ENEMY_FLAG_SUSPENDED;
                }

                script = get_script_by_id(enemy->auxScriptID);
                if (script != NULL) {
                    set_script_flags(script, EVT_FLAG_SUSPENDED);
                }
                script = get_script_by_id(enemy->aiScriptID);
                if (script != NULL) {
                    set_script_flags(script, EVT_FLAG_SUSPENDED);
                }

                if (enemy->flags & ENEMY_FLAG_DONT_SUSPEND_SCRIPTS) {
                    script = get_script_by_id(enemy->auxScriptID);
                    if (script != NULL) {
                        clear_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                    script = get_script_by_id(enemy->aiScriptID);
                    if (script != NULL) {
                        clear_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                }
            } else if (!(enemy->flags & ENEMY_FLAG_ACTIVE_WHILE_OFFSCREEN)) {
                get_screen_coords(gCurrentCameraID, npc->pos.x, npc->pos.y, npc->pos.z, &screenX, &screenY, &screenZ);
                if ((screenX < -160 || screenX > 480 || screenY < -120 || screenY > 360 || screenZ < 0) && !(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                    npc->flags |= NPC_FLAG_SUSPENDED;
                    enemy->flags |= ENEMY_FLAG_SUSPENDED;
                    script = get_script_by_id(enemy->auxScriptID);
                    if (script != NULL) {
                        set_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                    script = get_script_by_id(enemy->aiScriptID);
                    if (script != NULL) {
                        set_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                } else {
                    npc->flags &= ~NPC_FLAG_SUSPENDED;
                    enemy->flags &= ~ENEMY_FLAG_SUSPENDED;
                    script = get_script_by_id(enemy->auxScriptID);
                    if (script != NULL) {
                        clear_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                    script = get_script_by_id(enemy->aiScriptID);
                    if (script != NULL) {
                        clear_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                }
            } else {
                npc->flags &= ~NPC_FLAG_SUSPENDED;
                enemy->flags &= ~ENEMY_FLAG_SUSPENDED;
                script = get_script_by_id(enemy->auxScriptID);
                if (script != NULL) {
                    clear_script_flags(script, EVT_FLAG_SUSPENDED);
                }
                script = get_script_by_id(enemy->aiScriptID);
                if (script != NULL) {
                    clear_script_flags(script, EVT_FLAG_SUSPENDED);
                }
            }

            if (enemy->flags & ENEMY_FLAG_SUSPENDED) {
                continue;
            }
            if (enemy->flags & ENEMY_FLAG_PASSIVE) {
                if (!(enemy->flags & ENEMY_FLAG_DO_NOT_AUTO_FACE_PLAYER)) {
                    if (npc == playerStatus->encounteredNPC) {
                        enemy->savedNpcYaw = npc->yaw;
                        npc->yaw = atan2(npc->pos.x, npc->pos.z, playerStatus->pos.x, playerStatus->pos.z);
                        script = get_script_by_id(enemy->aiScriptID);
                        if (script != NULL) {
                            set_script_flags(script, EVT_FLAG_SUSPENDED);
                        }
                    } else {
                        if (enemy->savedNpcYaw != 12345) {
                            npc->yaw = enemy->savedNpcYaw;
                            enemy->savedNpcYaw = 12345;
                        }
                        script = get_script_by_id(enemy->aiScriptID);
                        if (script != NULL) {
                            clear_script_flags(script, EVT_FLAG_SUSPENDED);
                        }
                    }
                } else {
                    script = get_script_by_id(enemy->aiScriptID);
                    if (script != NULL) {
                        clear_script_flags(script, EVT_FLAG_SUSPENDED);
                    }
                }
            }

            if (currentEncounter->battleTriggerCooldown != 0
                || gGameStatusPtr->debugEnemyContact == DEBUG_CONTACT_CANT_TOUCH
                || (playerStatus->flags & PS_FLAG_ARMS_RAISED)
                || (gOverrideFlags & GLOBAL_OVERRIDES_40)
                || gPartnerStatus.actingPartner == PARTNER_BOW
                || (enemy->flags & ENEMY_FLAG_PASSIVE)
                || (gOverrideFlags & (GLOBAL_OVERRIDES_DISABLE_BATTLES | GLOBAL_OVERRIDES_200 | GLOBAL_OVERRIDES_400 | GLOBAL_OVERRIDES_800))
                || is_picking_up_item()
            ) {
                continue;
            }
            do {
                if (!(enemy->flags & ENEMY_FLAG_IGNORE_PARTNER) && partner_test_enemy_collision(npc)) {
                    currentEncounter->hitType = ENCOUNTER_TRIGGER_PARTNER;
                    enemy->encountered = ENCOUNTER_TRIGGER_PARTNER;
                    currentEncounter->curEncounter = encounter;
                    currentEncounter->curEnemy = enemy;
                    currentEncounter->firstStrikeType = FIRST_STRIKE_PLAYER;
                    goto START_BATTLE;
                }
            } while (0);

            npcX = npc->pos.x;
            npcY = npc->pos.y;
            npcZ = npc->pos.z;
            npcYaw = npc->yaw;
            colHeight = npc->collisionHeight;
            colRadius = npc->collisionDiameter / 2;

            if (enemy->unk_DC != 0) {
                npcYaw = npc->yawCamOffset;
                if (npcYaw < 180.0f) {
                    npcYaw = clamp_angle(camera->curYaw - 90.0f);
                } else {
                    npcYaw = clamp_angle(camera->curYaw + 90.0f);
                }

                add_vec2D_polar(&npcX, &npcZ, enemy->unk_DC, npcYaw);
            }

            dx = npcX - playerX;
            dz = npcZ - playerZ;
            distance = sqrtf(SQ(dx) + SQ(dz));

            switch (playerStatus->actionState) {
                case ACTION_STATE_HAMMER:
                    x = playerX;
                    y = playerY;
                    z = playerZ;
                    if (clamp_angle(playerStatus->spriteFacingAngle) < 180.0f) {
                        hammerDir = clamp_angle(camera->curYaw - 90.0f);
                        if (playerStatus->trueAnimation & SPRITE_ID_BACK_FACING) {
                            hammerDir = clamp_angle(hammerDir + 30.0f);
                        }
                    } else {
                        hammerDir = clamp_angle(camera->curYaw + 90.0f);
                        if (playerStatus->trueAnimation & SPRITE_ID_BACK_FACING) {
                            hammerDir = clamp_angle(hammerDir - 30.0f);
                        }
                    }
                    add_vec2D_polar(&x, &z, 24.0f, hammerDir);
                    dx = npcX - x;
                    dz = npcZ - z;
                    distance = sqrtf(SQ(dx) + SQ(dz));
                    if (enemy->flags & ENEMY_FLAG_IGNORE_HAMMER) {
                        break;
                    }
                    if (!(playerStatus->flags & PS_FLAG_HAMMER_CHECK)) {
                        break;
                    }
                    if (distance >= playerColRadius + colRadius || y > npcY + colHeight || npcY > y + playerColHeight) {
                        break;
                    }

                    testX = npcX;
                    testY = npcY;
                    testZ = npcZ;

                    if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(npcX, npcZ, playerX, playerZ), colHeight, colRadius * 2.0f)) {
                        testX = playerX;
                        testY = playerY;
                        testZ = playerZ;
                        if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(playerX, playerZ, npcX, npcZ), colHeight, colRadius * 2.0f)) {
                            break;
                        }
                    }
                    if (enemy->hitboxIsActive) {
                        npcX = enemy->unk_10.x;
                        npcY = enemy->unk_10.y;
                        npcZ = enemy->unk_10.z;
                    }

                    angle1 = fabsf(get_clamped_angle_diff(atan2(playerX, playerZ, npcX, npcZ), playerYaw));
                    angle2 = fabsf(get_clamped_angle_diff(atan2(npcX, npcZ, playerX, playerZ), npcYaw));
                    triggeredBattle = false;
                    if (angle1 >= 90.0f && angle2 >= 90.0f) {
                        triggeredBattle = false;
                    }
                    if (angle1 < 90.0f && angle2 >= 90.0f) {
                        triggeredBattle = true;
                    }
                    if (angle1 >= 90.0f && angle2 < 90.0f) {
                        triggeredBattle = false;
                    }
                    if (angle1 < 90.0f && angle2 < 90.0f) {
                        triggeredBattle = true;
                    }
                    if (triggeredBattle) {
                        sfx_play_sound_at_position(SOUND_HIT_PLAYER_NORMAL, SOUND_SPACE_DEFAULT, playerStatus->pos.x, playerStatus->pos.y, playerStatus->pos.z);
                        currentEncounter->hitType = ENCOUNTER_TRIGGER_HAMMER;
                        currentEncounter->hitTier = gPlayerData.hammerLevel;
                        enemy->encountered = ENCOUNTER_TRIGGER_HAMMER;
                        currentEncounter->curEncounter = encounter;
                        currentEncounter->curEnemy = enemy;
                        currentEncounter->firstStrikeType = FIRST_STRIKE_PLAYER;
                        goto START_BATTLE;
                    }
                    break;
                case ACTION_STATE_JUMP:
                case ACTION_STATE_BOUNCE:
                case ACTION_STATE_FALLING:
                case ACTION_STATE_STEP_DOWN:
                case ACTION_STATE_LAND:
                case ACTION_STATE_STEP_DOWN_LAND:
                case ACTION_STATE_SPIN_JUMP:
                case ACTION_STATE_SPIN_POUND:
                case ACTION_STATE_TORNADO_JUMP:
                case ACTION_STATE_TORNADO_POUND:
                    x = playerX;
                    z = playerZ;
                    if (!(enemy->flags & ENEMY_FLAG_IGNORE_JUMP)) {
                        if (distance >= playerColRadius + colRadius) {
                            continue;
                        }
                        if (playerY > npcY + colHeight || npcY > playerY + playerJumpColHeight) {
                            continue;
                        }

                        testX = npcX;
                        testY = npcY;
                        testZ = npcZ;
                        if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(npcX, npcZ, playerX, playerZ), colHeight, colRadius * 2.0f)) {
                            testX = playerX;
                            testY = playerY;
                            testZ = playerZ;
                            if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(playerX, playerZ, npcX, npcZ), colHeight, colRadius * 2.0f)) {
                                break;
                            }
                        }
                        triggeredBattle = false;
                        if (npcY + colHeight < playerY + playerJumpColHeight * 0.5f) {
                            if (playerStatus->actionState == ACTION_STATE_FALLING ||
                                playerStatus->actionState == ACTION_STATE_STEP_DOWN ||
                                playerStatus->actionState == ACTION_STATE_LAND ||
                                playerStatus->actionState == ACTION_STATE_STEP_DOWN_LAND ||
                                playerStatus->actionState == ACTION_STATE_SPIN_JUMP ||
                                playerStatus->actionState == ACTION_STATE_SPIN_POUND ||
                                playerStatus->actionState == ACTION_STATE_TORNADO_JUMP ||
                                playerStatus->actionState == ACTION_STATE_TORNADO_POUND) {
                                triggeredBattle = true;
                            }
                        }
                        if (playerY + playerJumpColHeight < npcY + colHeight * 0.5f) {
                            triggeredBattle = false;
                        }
                        if (triggeredBattle) {
                            if (gPlayerData.bootsLevel < 0) {
                                currentEncounter->hitType = ENCOUNTER_TRIGGER_NONE;
                                enemy->encountered = ENCOUNTER_TRIGGER_NONE;
                                currentEncounter->curEncounter = encounter;
                                currentEncounter->curEnemy = enemy;
                                currentEncounter->firstStrikeType = FIRST_STRIKE_NONE;
                                currentEncounter->hitTier = 0;
                                goto START_BATTLE;
                            }
                            currentEncounter->hitType = ENCOUNTER_TRIGGER_JUMP;
                            switch (playerStatus->actionState) {
                                case ACTION_STATE_JUMP:
                                case ACTION_STATE_BOUNCE:
                                case ACTION_STATE_FALLING:
                                case ACTION_STATE_STEP_DOWN:
                                case ACTION_STATE_LAND:
                                case ACTION_STATE_STEP_DOWN_LAND:
                                    currentEncounter->hitTier = 0;
                                    break;
                                case ACTION_STATE_SPIN_JUMP:
                                case ACTION_STATE_SPIN_POUND:
                                    currentEncounter->hitTier = 1;
                                    break;
                                case ACTION_STATE_TORNADO_JUMP:
                                case ACTION_STATE_TORNADO_POUND:
                                    currentEncounter->hitTier = 2;
                                    break;
                            }
                            sfx_play_sound_at_position(SOUND_HIT_PLAYER_NORMAL, SOUND_SPACE_DEFAULT, playerStatus->pos.x, playerStatus->pos.y, playerStatus->pos.z);
                            enemy->encountered = ENCOUNTER_STATE_NEUTRAL;
                            currentEncounter->curEncounter = encounter;
                            currentEncounter->curEnemy = enemy;
                            currentEncounter->firstStrikeType = FIRST_STRIKE_PLAYER;
                            goto START_BATTLE;
                        }
                    }
                    break;
            }

            if (enemy->flags & ENEMY_FLAG_IGNORE_TOUCH) {
                continue;
            }

            dx = npcX - playerX;
            dz = npcZ - playerZ;
            distance = sqrtf(SQ(dx) + SQ(dz));

            if (distance >= (playerColRadius + colRadius) * 0.8) {
                continue;
            }
            if (npcY + colHeight < playerY) {
                continue;
            }
            if (playerY + playerJumpColHeight < npcY) {
                continue;
            }

            testX = npcX;
            testY = npcY;
            testZ = npcZ;
            if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(npcX, npcZ, playerX, playerZ), colHeight, colRadius * 2.0f)) {
                testX = playerX;
                testY = playerY;
                testZ = playerZ;
                if (npc_test_move_taller_with_slipping(COLLIDER_FLAG_IGNORE_PLAYER, &testX, &testY, &testZ, distance, atan2(playerX, playerZ, npcX, npcZ), colHeight, colRadius * 2.0f)) {
                    continue;
                }
            }
            triggeredBattle = false;
            if (is_ability_active(ABILITY_SPIN_ATTACK) && gPlayerData.level >= enemy->npcSettings->level) {
                triggeredBattle = !currentEncounter->scriptedBattle;
            }
            if (is_ability_active(ABILITY_DIZZY_ATTACK)) {
                triggeredBattle = true;
            }
            if ((playerStatus->animFlags & PA_FLAG_SPINNING) && !(enemy->flags & ENEMY_FLAG_IGNORE_SPIN) && triggeredBattle) {
                sfx_play_sound_at_position(SOUND_HIT_PLAYER_NORMAL, SOUND_SPACE_DEFAULT, playerStatus->pos.x, playerStatus->pos.y, playerStatus->pos.z);
                testX = playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                currentEncounter->hitType = ENCOUNTER_TRIGGER_SPIN;
                playerStatus->animFlags |= PA_FLAG_DIZZY_ATTACK_ENCOUNTER;
                enemy->encountered = ENCOUNTER_TRIGGER_SPIN;
                currentEncounter->curEncounter = encounter;
                currentEncounter->curEnemy = enemy;
                currentEncounter->firstStrikeType = FIRST_STRIKE_NONE;
            } else {
                currentEncounter->hitType = ENCOUNTER_TRIGGER_NONE;
                playerStatus->animFlags &= ~PA_FLAG_DIZZY_ATTACK_ENCOUNTER;
                enemy->encountered = ENCOUNTER_TRIGGER_NONE;
                currentEncounter->curEncounter = encounter;
                currentEncounter->curEnemy = enemy;
                testX = playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                // if the hitbox is active, trigger a first strike
                firstStrikeType = FIRST_STRIKE_NONE;
                if (enemy->hitboxIsActive) {
                    if (is_ability_active(ABILITY_CHILL_OUT)) {
                        firstStrikeType = FIRST_STRIKE_NONE;
                    } else {
                        firstStrikeType = FIRST_STRIKE_ENEMY;
                    }
                }
                // cancel the first strike if bump attack is applicable
                if ((is_ability_active(ABILITY_BUMP_ATTACK)
                        && (gPlayerData.level >= enemy->npcSettings->level)
                        && !(enemy->flags & ENEMY_FLAG_PROJECTILE))
                    && !currentEncounter->scriptedBattle
                ) {
                    firstStrikeType = FIRST_STRIKE_NONE;
                }
                currentEncounter->firstStrikeType = firstStrikeType;
            }
            goto START_BATTLE;
        }
    }

START_BATTLE:
    switch (currentEncounter->hitType) {
        case 0:
            break;
        case ENCOUNTER_TRIGGER_NONE:
            currentEnemy = enemy = currentEncounter->curEnemy;
            if (enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            if (enemy->auxScript != NULL) {
                suspend_all_script(enemy->auxScriptID);
            }
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if ((currentEnemy->flags & ENEMY_FLAG_PROJECTILE) && enemy != currentEncounter->curEnemy) {
                    continue;
                }

                if (enemy->hitBytecode != NULL) {
                    enemy->encountered = ENCOUNTER_TRIGGER_NONE;
                    script = start_script(enemy->hitBytecode, EVT_PRIORITY_A, 0);
                    enemy->hitScript = script;
                    enemy->hitScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }
            }
            disable_player_input();
            partner_disable_input();
            if (playerStatus->actionState != ACTION_STATE_TORNADO_JUMP &&
                playerStatus->actionState != ACTION_STATE_TORNADO_POUND &&
                playerStatus->actionState != ACTION_STATE_SPIN_JUMP &&
                playerStatus->actionState != ACTION_STATE_SPIN_POUND) {
                playerStatus->flags |= PS_FLAG_ENTERING_BATTLE;
            }
            if (!is_ability_active(ABILITY_CHILL_OUT) && currentEncounter->firstStrikeType == FIRST_STRIKE_ENEMY) {
                set_action_state(ACTION_STATE_ENEMY_FIRST_STRIKE);
                npc = get_npc_unsafe(enemy->npcID);
                sfx_play_sound_at_position(SOUND_HIT_PLAYER_NORMAL, SOUND_SPACE_DEFAULT, npc->pos.x, npc->pos.y, npc->pos.z);
            }
            currentEncounter->scriptedBattle = false;
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
            break;
        case ENCOUNTER_TRIGGER_SPIN:
            currentEnemy = enemy = currentEncounter->curEnemy;
            if (enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            if (enemy->auxScript != NULL) {
                suspend_all_script(enemy->auxScriptID);
            }
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if ((currentEnemy->flags & ENEMY_FLAG_PROJECTILE) && enemy != currentEncounter->curEnemy) {
                    continue;
                }

                if (enemy->hitBytecode != NULL) {
                    enemy->encountered = ENCOUNTER_TRIGGER_SPIN;
                    script = start_script(enemy->hitBytecode, EVT_PRIORITY_A, 0);
                    enemy->hitScript = script;
                    enemy->hitScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }
            }
            disable_player_input();
            partner_disable_input();
            currentEncounter->scriptedBattle = false;
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
            playerStatus->flags |= PS_FLAG_ENTERING_BATTLE;
            break;
        case ENCOUNTER_TRIGGER_JUMP:
            currentEnemy = enemy = currentEncounter->curEnemy;
            if (enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            if (enemy->auxScript != NULL) {
                suspend_all_script(enemy->auxScriptID);
            }
            encounter = currentEncounter->curEncounter;

            cond2 = false;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if ((currentEnemy->flags & ENEMY_FLAG_PROJECTILE) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->hitBytecode != NULL) {
                    enemy->encountered = ENCOUNTER_TRIGGER_JUMP;
                    script = start_script(enemy->hitBytecode, EVT_PRIORITY_A, 0);
                    enemy->hitScript = script;
                    enemy->hitScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                    npc = get_npc_unsafe(enemy->npcID);
                    cond2 = true;
                    testX =  playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                    testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                    testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                    fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                } else if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                    npc = get_npc_unsafe(enemy->npcID);
                    cond2 = true;
                    testX =  playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                    testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                    testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                    fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                }
            }
            disable_player_input();
            partner_disable_input();
            playerStatus->flags |= PS_FLAG_ENTERING_BATTLE;
            if (cond2) {
                start_bounce_a();
            }
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            currentEncounter->scriptedBattle = false;
            sfx_play_sound(SOUND_NONE);
            gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
            break;
        case ENCOUNTER_TRIGGER_HAMMER:
            currentEnemy = enemy = currentEncounter->curEnemy;
            if (enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            if (enemy->auxScript != NULL) {
                suspend_all_script(enemy->auxScriptID);
            }
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if ((currentEnemy->flags & ENEMY_FLAG_PROJECTILE) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->hitBytecode != NULL) {
                    enemy->encountered = ENCOUNTER_TRIGGER_HAMMER;
                    script = start_script(enemy->hitBytecode, EVT_PRIORITY_A, 0);
                    enemy->hitScript = script;
                    enemy->hitScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                    npc = get_npc_unsafe(enemy->npcID);
                    testX =  playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                    testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                    testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                    fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                } else if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                    npc = get_npc_unsafe(enemy->npcID);
                    testX =  playerStatus->pos.x + ((npc->pos.x - playerStatus->pos.x) * 0.5f);
                    testY = playerStatus->pos.y + (((npc->pos.y + npc->collisionHeight) - (playerStatus->pos.y + playerStatus->colliderHeight)) * 0.5f);
                    testZ = playerStatus->pos.z + ((npc->pos.z - playerStatus->pos.z) * 0.5f);
                    fx_damage_stars(FX_DAMAGE_STARS_3, npc->pos.x, npc->pos.y + npc->collisionHeight, npc->pos.z, 0.0f, -1.0f, 0.0f, 3);
                }
            }
            disable_player_input();
            partner_disable_input();
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            currentEncounter->scriptedBattle = false;
            playerStatus->flags |= PS_FLAG_ENTERING_BATTLE;
            sfx_play_sound(SOUND_NONE);
            gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
            break;
        case ENCOUNTER_TRIGGER_CONVERSATION:
            suspend_all_group(EVT_GROUP_FLAG_INTERACT);
            enemy = currentEncounter->curEnemy;
            if (enemy != NULL && enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            enemy = currentEncounter->curEnemy;
            if (enemy->interactBytecode != NULL) {
                enemy->encountered = ENCOUNTER_TRIGGER_CONVERSATION;
                script = start_script(enemy->interactBytecode, EVT_PRIORITY_A, 0);
                enemy->interactScript = script;
                enemy->interactScriptID = script->id;
                script->owner1.enemy = enemy;
                script->owner2.npcID = enemy->npcID;
                script->groupFlags = enemy->scriptGroup;
            }
            disable_player_input();
            partner_disable_input();
            set_action_state(ACTION_STATE_TALK);
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            func_800EF3D4(1);
            gEncounterState = ENCOUNTER_STATE_CONVERSATION;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_CONVERSATION_INIT;
            break;
        case ENCOUNTER_TRIGGER_PARTNER:
            currentEnemy = enemy = currentEncounter->curEnemy;
            if (enemy->aiScript != NULL) {
                suspend_all_script(enemy->aiScriptID);
            }
            if (enemy->auxScript != NULL) {
                suspend_all_script(enemy->auxScriptID);
            }
            encounter = currentEncounter->curEncounter;

            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if ((currentEnemy->flags & ENEMY_FLAG_PROJECTILE) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->hitBytecode != NULL) {
                    enemy->encountered = ENCOUNTER_TRIGGER_PARTNER;
                    script = start_script(enemy->hitBytecode, EVT_PRIORITY_A, 0);
                    enemy->hitScript = script;
                    enemy->hitScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                    npc = get_npc_unsafe(enemy->npcID);
                    testX = npc->pos.x;
                    testY = npc->pos.y + npc->collisionHeight;
                    testZ = npc->pos.z;
                    fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                } else if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                    npc = get_npc_unsafe(enemy->npcID);
                    testX = npc->pos.x;
                    testY = npc->pos.y + npc->collisionHeight;
                    testZ = npc->pos.z;
                    fx_damage_stars(FX_DAMAGE_STARS_3, testX, testY, testZ, 0.0f, -1.0f, 0.0f, 3);
                }
            }
            disable_player_input();
            partner_disable_input();
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 0;
            currentEncounter->scriptedBattle = false;
            playerStatus->flags |= PS_FLAG_ENTERING_BATTLE;
            sfx_play_sound(SOUND_NONE);
            gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
            break;
    }
}

void draw_encounters_neutral(void) {
}

void update_encounters_pre_battle(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    PlayerData* playerData = &gPlayerData;
    Encounter* encounter;
    Enemy* enemy;
    s32 i;
    s32 j;

    switch (gEncounterSubState) {
        case ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT:
#if VERSION_PAL
            gPlayerStatusPtr->pitch = 0;
#endif
            currentEncounter->fadeOutAmount = 0;
            currentEncounter->substateDelay = 1;
            currentEncounter->fadeOutAccel = 1;
            currentEncounter->unk_08 = -1;
            HasPreBattleSongPushed = false;
            D_80077C40 = false;
            suspend_all_group(EVT_GROUP_FLAG_BATTLE);

            // suspend all ai scripts
            for (i = 0; i < currentEncounter->numEncounters; i++) {
                encounter = currentEncounter->encounterList[i];

                if (encounter != NULL) {
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy != NULL && !(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                            if (enemy->aiScript != NULL) {
                                suspend_all_script(enemy->aiScriptID);
                            }
                            if (enemy->auxScript != NULL) {
                                suspend_all_script(enemy->auxScriptID);
                            }
                        }
                    }
                }
            }

            // try skip-on-contact
            enemy = currentEncounter->curEnemy;
            if ((enemy->flags & ENEMY_FLAG_SKIP_BATTLE) && !currentEncounter->scriptedBattle) {
                currentEncounter->substateDelay = 0;
                currentEncounter->battleStartCountdown = 0;
                partner_handle_before_battle();
                gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_SKIP;
                return;
            }

            // try kill-on-contact
            if (gGameStatusPtr->debugEnemyContact == DEBUG_CONTACT_DIE_ON_TOUCH) {
                currentEncounter->substateDelay = 0;
                currentEncounter->battleStartCountdown = 10;
                partner_handle_before_battle();
                gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_AUTO_WIN;
                return;
            }

            // try first attack kill
            enemy = currentEncounter->curEnemy;
            if (currentEncounter->hitType != ENCOUNTER_TRIGGER_NONE
                && currentEncounter->hitType != ENCOUNTER_TRIGGER_SPIN
                && is_ability_active(ABILITY_FIRST_ATTACK)
                && (playerData->level >= enemy->npcSettings->level)
                && !(enemy->flags & ENEMY_FLAG_PROJECTILE)
                && !currentEncounter->scriptedBattle
            ) {
                currentEncounter->substateDelay = 0;
                currentEncounter->battleStartCountdown = 10;
                D_80077C40 = true;
                gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_AUTO_WIN;
                return;
            }

            // try bump attack kill
            enemy = currentEncounter->curEnemy;
            if (is_ability_active(ABILITY_BUMP_ATTACK)
                && (playerData->level >= enemy->npcSettings->level)
                && !(enemy->flags & ENEMY_FLAG_PROJECTILE)
                && !(currentEncounter->scriptedBattle)
            ) {
                currentEncounter->substateDelay = 0;
                currentEncounter->battleStartCountdown = 10;
                D_80077C40 = true;
                gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_AUTO_WIN;
                return;
            }

            // try spin attack kill
            enemy = currentEncounter->curEnemy;
            if (currentEncounter->hitType == ENCOUNTER_TRIGGER_SPIN
                && is_ability_active(ABILITY_SPIN_ATTACK)
                && playerData->level >= enemy->npcSettings->level
                && !(enemy->flags & ENEMY_FLAG_PROJECTILE)
                && !currentEncounter->scriptedBattle
            ) {
                currentEncounter->substateDelay = 0;
                currentEncounter->battleStartCountdown = 10;
                D_80077C40 = true;
                gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_AUTO_WIN;
                return;
            }

            // start battle music
            if (currentEncounter->songID <= AU_SONG_NONE) {
                switch (currentEncounter->firstStrikeType) {
                    case FIRST_STRIKE_NONE:
                        bgm_set_battle_song(SONG_NORMAL_BATTLE, FIRST_STRIKE_NONE);
                        break;
                    case FIRST_STRIKE_PLAYER:
                        bgm_set_battle_song(SONG_NORMAL_BATTLE, FIRST_STRIKE_PLAYER);
                        break;
                    case FIRST_STRIKE_ENEMY:
                        bgm_set_battle_song(SONG_NORMAL_BATTLE, FIRST_STRIKE_ENEMY);
                        break;
                }
            } else {
                bgm_set_battle_song(currentEncounter->songID, BGM_VARIATION_0);
            }
            bgm_push_battle_song();
            HasPreBattleSongPushed = true;

            currentEncounter->battleStartCountdown = 10;
            gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_LOAD;
            // fallthrough
        case ENCOUNTER_SUBSTATE_PRE_BATTLE_LOAD:
            // wait for screen to fade out
            if (currentEncounter->fadeOutAmount != 255) {
                break;
            }
            // delay before loading the battle
            if (currentEncounter->battleStartCountdown != 0) {
                currentEncounter->battleStartCountdown--;
                break;
            }

            // kill all enemy hit scripts
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy != NULL &&
                    ((!(enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) || enemy == currentEncounter->curEnemy)) &&
                    !(enemy->flags & ENEMY_FLAG_DISABLE_AI) &&
                    enemy->hitScript != NULL)
                {
                    kill_script_by_ID(enemy->hitScriptID);
                    enemy->hitScript = NULL;
                }
            }

            partner_handle_before_battle();
            currentEncounter->dizzyAttack.status = 0;
            currentEncounter->dizzyAttack.duration = 0;

            enemy = currentEncounter->curEnemy;
            currentEncounter->instigatorValue = enemy->instigatorValue;

            if (is_ability_active(ABILITY_DIZZY_ATTACK) && currentEncounter->hitType == ENCOUNTER_TRIGGER_SPIN) {
                currentEncounter->dizzyAttack.status = 4;
                currentEncounter->dizzyAttack.duration = 3;
            }

            sfx_stop_sound(SOUND_SPIN);
            sfx_stop_sound(SOUND_SPEEDY_SPIN);
            sfx_stop_sound(SOUND_SPIN_ATTACK);
            sfx_stop_sound(SOUND_SPEEDY_SPIN_ATTACK);
            set_battle_formation(NULL);
            set_battle_stage(encounter->stage);
            load_battle(encounter->battle);
            currentEncounter->unk_07 = 1;
            currentEncounter->unk_08 = 0;
            currentEncounter->hasMerleeCoinBonus = false;
            currentEncounter->damageTaken = 0;
            currentEncounter->coinsEarned = 0;
            currentEncounter->fadeOutAccel = 0;
            currentEncounter->fadeOutAmount = 255;
            set_screen_overlay_params_front(OVERLAY_SCREEN_COLOR, 255.0f);

            // prepare to resume after battle
            gEncounterState = ENCOUNTER_STATE_POST_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_INIT;
            break;
        case ENCOUNTER_SUBSTATE_PRE_BATTLE_AUTO_WIN:
            if (currentEncounter->battleStartCountdown != 0) {
                currentEncounter->battleStartCountdown--;
                break;
            }

            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy != NULL &&
                    (!(enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) || enemy == currentEncounter->curEnemy) &&
                    !(enemy->flags & ENEMY_FLAG_DISABLE_AI) &&
                    (enemy->hitScript != 0))
                {
                    kill_script_by_ID(enemy->hitScriptID);
                    enemy->hitScript = NULL;
                }
            }

            currentEncounter->unk_08 = 1;
            currentEncounter->unk_07 = 1;
            currentEncounter->battleOutcome = OUTCOME_PLAYER_WON;
            currentEncounter->hasMerleeCoinBonus = false;
            currentEncounter->damageTaken = 0;
            currentEncounter->coinsEarned = 0;

            currentEncounter->fadeOutAccel = 0;
            currentEncounter->fadeOutAmount = 0;
            gEncounterState = ENCOUNTER_STATE_POST_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_INIT;
            break;
        case ENCOUNTER_SUBSTATE_PRE_BATTLE_SKIP:
            currentEncounter->battleOutcome = OUTCOME_SKIP;
            currentEncounter->unk_08 = 1;

            currentEncounter->fadeOutAmount = 0;
            currentEncounter->fadeOutAccel = 0;
            gEncounterState = ENCOUNTER_STATE_POST_BATTLE;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_INIT;
            break;
    }
}

void draw_encounters_pre_battle(void) {
    EncounterStatus* encounter = &gCurrentEncounter;
    Npc* npc = get_npc_unsafe(encounter->curEnemy->npcID);
    PlayerStatus* playerStatus = &gPlayerStatus;

    if (encounter->substateDelay != 0) {
        f32 playerX, playerY, playerZ;
        f32 otherX, otherY, otherZ;
        s32 pScreenX, pScreenY, pScreenZ;
        s32 oScreenX, oScreenY, oScreenZ;

        if (encounter->fadeOutAmount != 255) {
            encounter->fadeOutAccel++;
            if (encounter->fadeOutAccel > 10) {
                encounter->fadeOutAccel = 10;
            }

            encounter->fadeOutAmount += encounter->fadeOutAccel;
            if (encounter->fadeOutAmount > 255) {
                encounter->fadeOutAmount = 255;
            }

            playerX = playerStatus->pos.x;
            playerY = playerStatus->pos.y;
            playerZ = playerStatus->pos.z;

            otherX = npc->pos.x;
            otherY = npc->pos.y;
            otherZ = npc->pos.z;
            if (otherY < -990.0f) {
                otherX = playerX;
                otherY = playerY;
                otherZ = playerZ;
            }

            if (gGameStatusPtr->demoState == DEMO_STATE_CHANGE_MAP) {
                set_screen_overlay_params_back(OVERLAY_START_BATTLE, encounter->fadeOutAmount);
                set_screen_overlay_alpha(SCREEN_LAYER_BACK, 255.0f);
                set_screen_overlay_color(SCREEN_LAYER_BACK, 0, 0, 0);
                get_screen_coords(gCurrentCameraID, playerX, playerY + 20.0f, playerZ, &pScreenX, &pScreenY, &pScreenZ);
                get_screen_coords(gCurrentCameraID, otherX, otherY + 15.0f, otherZ, &oScreenX, &oScreenY, &oScreenZ);
                set_screen_overlay_center(SCREEN_LAYER_BACK, 0, (pScreenX - oScreenX) / 2 + oScreenX,
                                              (pScreenY - oScreenY) / 2 + oScreenY);
            } else {
                set_screen_overlay_params_front(OVERLAY_START_BATTLE, encounter->fadeOutAmount);
                set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 255.0f);
                set_screen_overlay_color(SCREEN_LAYER_FRONT, 0, 0, 0);
                get_screen_coords(gCurrentCameraID, playerX, playerY + 20.0f, playerZ, &pScreenX, &pScreenY, &pScreenZ);
                get_screen_coords(gCurrentCameraID, otherX, otherY + 15.0f, otherZ, &oScreenX, &oScreenY, &oScreenZ);
                set_screen_overlay_center(SCREEN_LAYER_FRONT, 0, (pScreenX - oScreenX) / 2 + oScreenX,
                                              (pScreenY - oScreenY) / 2 + oScreenY);
            }
        }
    }
}

void show_first_strike_message(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    s32 posX;
    s32 width;
    s32 xOffset;
    s32 screenWidthHalf;

    if (currentEncounter->substateDelay == 0) {
        gFirstStrikeMessagePos = -200;
        return;
    }

    gFirstStrikeMessagePos += 40;
    xOffset = gFirstStrikeMessagePos;
    if (xOffset > 0) {
        if (xOffset < 1600) {
            xOffset = 0;
        } else {
            xOffset -= 1600;
        }
    }

    screenWidthHalf = SCREEN_WIDTH / 2;

    switch (currentEncounter->firstStrikeType) {
        case FIRST_STRIKE_PLAYER:
            switch (currentEncounter->hitType) {
                case 2:
                case 4:
                    width = get_msg_width(MSG_Menus_PlayerFirstStrike, 0) + 24;
                    posX = (xOffset + screenWidthHalf) - (width / 2);
                    draw_box(0, WINDOW_STYLE_20, posX, 69, 0, width, 28, 255, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0, NULL,
                             SCREEN_WIDTH, SCREEN_HEIGHT, NULL);
                    draw_msg(MSG_Menus_PlayerFirstStrike, posX + 11, 75, 0xFF, MSG_PAL_STANDARD, 0);
                    break;
                case 6:
                    width = get_msg_width(MSG_Menus_PartnerFirstStrike, 0) + 24;
                    posX = (xOffset + screenWidthHalf) - (width / 2);
                    draw_box(0, WINDOW_STYLE_20, posX, 69, 0, width, 28, 255, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0, NULL,
                             SCREEN_WIDTH, SCREEN_HEIGHT, NULL);
                    draw_msg(MSG_Menus_PartnerFirstStrike, posX + 11, 75, 0xFF, MSG_PAL_STANDARD, 0);
                    break;
            }
            break;
        case FIRST_STRIKE_ENEMY:
            if (!is_ability_active(ABILITY_CHILL_OUT)) {
                width = get_msg_width(MSG_Menus_EnemyFirstStrike, 0) + 24;
                posX = (xOffset + screenWidthHalf) - (width / 2);
                draw_box(0, WINDOW_STYLE_4, posX, 69, 0, width, 28, 255, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0, NULL,
                         SCREEN_WIDTH, SCREEN_HEIGHT, NULL);
                draw_msg(MSG_Menus_EnemyFirstStrike, posX + 11, 75, 0xFF, MSG_PAL_STANDARD, 0);
            }
            break;
    }
}

void update_encounters_post_battle(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    PlayerStatus* playerStatus = &gPlayerStatus;
    PlayerData* playerData = &gPlayerData;
    PartnerStatus* partnerStatus = &gPartnerStatus;
    Encounter* encounter;
    Evt* script;
    Enemy* enemy;
    s32 i, j;
    s32 hasDefeatScript;
    Npc* npc;

    switch (gEncounterSubState) {
        case ENCOUNTER_SUBSTATE_POST_BATTLE_INIT:
            if (currentEncounter->unk_08 == 0) {
                return;
            }

            currentEncounter->unk_08 = 0;
            gPlayerStatus.blinkTimer = 0;
            currentEncounter->scriptedBattle = false;
            setup_status_bar_for_world();
            currentEncounter->dizzyAttack.status = 0;
            currentEncounter->unusedAttack1.status = 0;
            currentEncounter->unusedAttack2.status = 0;
            currentEncounter->unusedAttack3.status = 0;
            currentEncounter->dizzyAttack.duration = 0;
            currentEncounter->unusedAttack1.duration = 0;
            currentEncounter->unusedAttack2.duration = 0;
            currentEncounter->unusedAttack3.duration = 0;
            if (HasPreBattleSongPushed == true) {
                bgm_pop_battle_song();
            }
            currentEncounter->fadeOutAccel = 1;
            currentEncounter->battleStartCountdown = 0;
            LastBattleStartedBySpin = false;
            gPlayerStatus.flags &= ~PS_FLAG_ENTERING_BATTLE;
            if (currentEncounter->hitType == ENCOUNTER_TRIGGER_SPIN) {
                LastBattleStartedBySpin = true;
            }
            currentEncounter->hitType = 0;
            if (!D_80077C40) {
                partner_handle_after_battle();
            }
            PendingPartnerAbilityResume = false;
            if (partnerStatus->shouldResumeAbility) {
                PendingPartnerAbilityResume = true;
            } else if (!LastBattleStartedBySpin
                && !(gPlayerStatus.flags & (PS_FLAG_JUMPING | PS_FLAG_FALLING))
                && gPlayerStatus.actionState != ACTION_STATE_RIDE
                && gPlayerStatus.actionState != ACTION_STATE_USE_SPINNING_FLOWER
            ) {
                set_action_state(ACTION_STATE_IDLE);
            }
            switch (currentEncounter->battleOutcome) {
                case OUTCOME_PLAYER_WON:
                    gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_WON_CHECK_MERLEE;
                    break;
                case OUTCOME_PLAYER_LOST:
                    gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_INIT;
                    break;
                case OUTCOME_PLAYER_FLED:
                    gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_INIT;
                    break;
                case OUTCOME_SKIP:
                    gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_SKIP;
                    break;
                case OUTCOME_ENEMY_FLED:
                    gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_INIT;
                    break;
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_WON_CHECK_MERLEE:
            if (currentEncounter->hasMerleeCoinBonus) {
                if (get_coin_drop_amount(currentEncounter->curEnemy) != 0) {
                    MerleeDropCoinsEvt = start_script(&EVS_MerleeDropCoins, EVT_PRIORITY_A, 0);
                    MerleeDropCoinsEvt->groupFlags = EVT_GROUP_NEVER_PAUSE;
                    MerleeDropCoinsEvtID = MerleeDropCoinsEvt->id;
                } else {
                    playerData->merleeTurnCount = 0;
                    playerData->merleeCastsLeft++;
                }
            }
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_PLAY_NPC_DEFEAT;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_PLAY_NPC_DEFEAT:
            // fade screen in and wait for merlee bonus to finish (if applicable)
            if (currentEncounter->hasMerleeCoinBonus) {
                if (get_coin_drop_amount(currentEncounter->curEnemy) != 0) {
                    currentEncounter->fadeOutAccel += 4;
                    currentEncounter->fadeOutAmount -= currentEncounter->fadeOutAccel;
                    if (currentEncounter->fadeOutAmount < 0) {
                        currentEncounter->fadeOutAmount = 0;
                    }
                    if (does_script_exist(MerleeDropCoinsEvtID)) {
                        break;
                    }
                }
            }
            // start defeat scripts for current enemy
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if (enemy->defeatBytecode != NULL) {
                    script = start_script_in_group(enemy->defeatBytecode, EVT_PRIORITY_A, 0, EVT_GROUP_NEVER_PAUSE);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = EVT_GROUP_NEVER_PAUSE;
                    currentEncounter->battleStartCountdown = 1;
                } else {
                    script = start_script_in_group(&EVS_NpcDefeat, EVT_PRIORITY_A, 0, EVT_GROUP_NEVER_PAUSE);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = EVT_GROUP_NEVER_PAUSE;
                }
            }
            if (!(currentEncounter->flags & ENCOUNTER_FLAG_THUMBS_UP)
                && !PendingPartnerAbilityResume
                && currentEncounter->battleStartCountdown == 0
                && !LastBattleStartedBySpin
            ) {
                suggest_player_anim_allow_backward(ANIM_Mario1_ThumbsUp);
            }
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_WON_FADE_IN;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_WON_FADE_IN:
            if (currentEncounter->fadeOutAmount == 0) {
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_WON_KILL;
            } else {
                currentEncounter->fadeOutAccel += 4;
                currentEncounter->fadeOutAmount -= currentEncounter->fadeOutAccel;
                if (currentEncounter->fadeOutAmount < 0) {
                    currentEncounter->fadeOutAmount = 0;
                }
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_WON_KILL:
            // wait for all defeat scripts to finish
            hasDefeatScript = false;
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }
                if (does_script_exist(enemy->defeatScriptID)) {
                    hasDefeatScript = true;
                } else {
                    enemy->defeatScript = NULL;
                }
            }
            // kill defeated enemies
            if (!hasDefeatScript) {
                if (!(currentEncounter->flags & ENCOUNTER_FLAG_THUMBS_UP)
                    && !PendingPartnerAbilityResume
                    && currentEncounter->battleStartCountdown == 1
                ) {
                    suggest_player_anim_allow_backward(ANIM_Mario1_ThumbsUp);
                }
                encounter = currentEncounter->curEncounter;
                for (i = 0; i < encounter->count; i++) {
                    enemy = encounter->enemy[i];
                    enemy = encounter->enemy[i];
                    if (enemy == NULL) {
                        continue;
                    }
                    if (enemy->flags & ENEMY_FLAG_DO_NOT_KILL) {
                        continue;
                    }
                    if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                        continue;
                    }
                    if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                        if (!(enemy->flags & ENEMY_FLAG_FLED)) {
                            set_defeated(currentEncounter->mapID, encounter->encounterID + i);
                        }
                    }
                    kill_enemy(enemy);
                }

                currentEncounter->substateDelay = 0;
                if (!(currentEncounter->flags & ENCOUNTER_FLAG_THUMBS_UP)
                    && !PendingPartnerAbilityResume
                    && currentEncounter->battleStartCountdown == 1
                ) {
                    currentEncounter->substateDelay = 30;
                }
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_WON_RESUME;
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_WON_RESUME:
            if (!(currentEncounter->flags & ENCOUNTER_FLAG_CANT_SKIP_WIN_DELAY)) {
                if (gGameStatusPtr->stickX[0] != 0 || gGameStatusPtr->stickY[0] != 0) {
                    currentEncounter->substateDelay = 0;
                }
            }
            if (currentEncounter->substateDelay != 0) {
                currentEncounter->substateDelay--;
                break;
            }

            for (i = 0; i < currentEncounter->numEncounters; i++) {
                encounter = currentEncounter->encounterList[i];
                if (encounter == NULL) {
                    continue;
                }
                for (j = 0; j < encounter->count; j++) {
                    enemy = encounter->enemy[j];
                    if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                        continue;
                    }
                    if (enemy->aiScript != NULL) {
                        resume_all_script(enemy->aiScriptID);
                    }
                    if (enemy->auxScript != NULL) {
                        resume_all_script(enemy->auxScriptID);
                    }
                }
            }

            currentEncounter->battleTriggerCooldown = 15;
            enable_player_input();
            partner_enable_input();
            if (!PendingPartnerAbilityResume) {
                suggest_player_anim_allow_backward(ANIM_Mario1_Idle);
            }
            set_screen_overlay_params_front(OVERLAY_NONE, -1.0f);
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_INIT:
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }
                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }

                if (enemy->defeatBytecode != NULL) {
                    script = start_script(enemy->defeatBytecode, EVT_PRIORITY_A, 0);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    enemy->aiFlags |= AI_FLAG_1;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }
            }
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_FADE_IN;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_FADE_IN:
            if (currentEncounter->fadeOutAmount == 0) {
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_RESUME;
            } else {
                currentEncounter->fadeOutAccel += 4;
                currentEncounter->fadeOutAmount -= currentEncounter->fadeOutAccel;
                if (currentEncounter->fadeOutAmount < 0) {
                    currentEncounter->fadeOutAmount = 0;
                }
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_RESUME:
            encounter = currentEncounter->curEncounter;
            hasDefeatScript = false;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if (!(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                    if (does_script_exist(enemy->defeatScriptID)) {
                        hasDefeatScript = true;
                    } else {
                        enemy->defeatScript = NULL;
                    }
                }
            }
            if (!hasDefeatScript) {
                for (i = 0; i < currentEncounter->numEncounters; i++) {
                    encounter = currentEncounter->encounterList[i];
                    if (encounter == NULL) {
                        continue;
                    }
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                            continue;
                        }
                        if (enemy->aiScript != NULL) {
                            resume_all_script(enemy->aiScriptID);
                        }
                        if (enemy->auxScript != NULL) {
                            resume_all_script(enemy->auxScriptID);
                        }
                    }
                }

                enemy = currentEncounter->curEnemy;
                encounter = currentEncounter->curEncounter;
                if (!(enemy->flags & ENEMY_FLAG_NO_DELAY_AFTER_FLEE)) {
                    enemy->aiSuspendTime = 45;
                    playerStatus->blinkTimer = 45;
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy == NULL) {
                            continue;
                        }
                        if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                            continue;
                        }
                        if (enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) {
                            continue;
                        }
                        enemy->aiSuspendTime = 45;
                        playerStatus->blinkTimer = 45;
                    }
                }

                enemy = currentEncounter->curEnemy;
                if (!(currentEncounter->flags & ENCOUNTER_FLAG_SKIP_FLEE_DROPS)) {
                    script = start_script(&EVS_FleeBattleDrops, EVT_PRIORITY_A, 0);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }

                currentEncounter->battleTriggerCooldown = 45;
                playerStatus->blinkTimer = 45;
                enable_player_input();
                partner_enable_input();
                set_screen_overlay_params_front(OVERLAY_NONE, -1.0f);
                if (!PendingPartnerAbilityResume) {
                    currentEncounter->substateDelay = 15;
                } else {
                    currentEncounter->substateDelay = 0;
                }
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_DELAY;
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_FLED_DELAY:
            if (currentEncounter->substateDelay != 0) {
                currentEncounter->substateDelay--;
                if (gGameStatusPtr->curButtons[0] == 0 && gGameStatusPtr->stickX[0] == 0 && gGameStatusPtr->stickY[0] == 0) {
                    break;
                }
            }
            if (!PendingPartnerAbilityResume && playerStatus->anim == ANIM_MarioB3_Hustled) {
                suggest_player_anim_allow_backward(ANIM_Mario1_Idle);
            }
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_INIT:
            suggest_player_anim_allow_backward(ANIM_MarioW2_LayingDown);
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }

                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }

                if (enemy->defeatBytecode != NULL) {
                    script = start_script(enemy->defeatBytecode, EVT_PRIORITY_A, 0);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }
            }
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_FADE_IN;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_FADE_IN:
            if (currentEncounter->fadeOutAmount == 0) {
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_RESUME;
            } else {
                currentEncounter->fadeOutAccel += 4;
                currentEncounter->fadeOutAmount -= currentEncounter->fadeOutAccel;
                if (currentEncounter->fadeOutAmount < 0) {
                    currentEncounter->fadeOutAmount = 0;
                }
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_RESUME:
            hasDefeatScript = false;
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if (!(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                    if (does_script_exist(enemy->defeatScriptID)) {
                        hasDefeatScript = true;
                    } else {
                        enemy->defeatScript = NULL;
                    }
                }
            }
            if (!hasDefeatScript) {
                for (i = 0; i < currentEncounter->numEncounters; i++) {
                    encounter = currentEncounter->encounterList[i];
                    if (encounter == NULL) {
                        continue;
                    }
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                            continue;
                        }
                        if (enemy->aiScript != NULL) {
                            resume_all_script(enemy->aiScriptID);
                        }
                        if (enemy->auxScript != NULL) {
                            resume_all_script(enemy->auxScriptID);
                        }
                    }
                }
                enable_player_input();
                partner_enable_input();
                set_screen_overlay_params_front(OVERLAY_NONE, -1.0f);
                currentEncounter->substateDelay = 15;
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_DELAY;
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_LOST_DELAY:
            if (currentEncounter->substateDelay != 0) {
                currentEncounter->substateDelay--;
                if (gGameStatusPtr->curButtons[0] == 0 && gGameStatusPtr->stickX[0] == 0 && gGameStatusPtr->stickY[0] == 0) {
                    break;
                }
            }
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_SKIP:
            // resume all ai scripts
            for (i = 0; i < currentEncounter->numEncounters; i++) {
                encounter = currentEncounter->encounterList[i];
                if (encounter == NULL) {
                    continue;
                }
                for (j = 0; j < encounter->count; j++) {
                    enemy = encounter->enemy[j];
                    if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                        continue;
                    }
                    if (enemy->aiScript != NULL) {
                        resume_all_script(enemy->aiScriptID);
                    }
                    if (enemy->auxScript != NULL) {
                        resume_all_script(enemy->auxScriptID);
                    }
                }
            }
            enable_player_input();
            partner_enable_input();
            set_screen_overlay_params_front(OVERLAY_NONE, -1.0f);
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_INIT:
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if ((enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && enemy != currentEncounter->curEnemy) {
                    continue;
                }

                if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                    continue;
                }

                if (enemy->defeatBytecode != NULL) {
                    script = start_script(enemy->defeatBytecode, EVT_PRIORITY_A, 0);
                    enemy->defeatScript = script;
                    enemy->defeatScriptID = script->id;
                    script->owner1.enemy = enemy;
                    script->owner2.npcID = enemy->npcID;
                    script->groupFlags = enemy->scriptGroup;
                }
            }
            gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_FADE_IN;
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_FADE_IN:
            if (currentEncounter->fadeOutAmount == 0) {
                gEncounterSubState = ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_RESUME;
            } else {
                currentEncounter->fadeOutAccel += 4;
                currentEncounter->fadeOutAmount -= currentEncounter->fadeOutAccel;
                if (currentEncounter->fadeOutAmount < 0) {
                    currentEncounter->fadeOutAmount = 0;
                }
            }
            break;
        case ENCOUNTER_SUBSTATE_POST_BATTLE_ENEMY_FLED_RESUME:
            hasDefeatScript = false;
            encounter = currentEncounter->curEncounter;
            for (i = 0; i < encounter->count; i++) {
                enemy = encounter->enemy[i];
                if (enemy == NULL) {
                    continue;
                }
                if (!(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                    if (does_script_exist(enemy->defeatScriptID)) {
                        hasDefeatScript = true;
                    } else {
                        enemy->defeatScript = NULL;
                    }
                }
            }
            if (!hasDefeatScript) {
                for (i = 0; i < currentEncounter->numEncounters; i++) {
                    encounter = currentEncounter->encounterList[i];
                    if (encounter == NULL) {
                        continue;
                    }
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                            continue;
                        }
                        if (enemy->aiScript != NULL) {
                            resume_all_script(enemy->aiScriptID);
                        }
                        if (enemy->auxScript != NULL) {
                            resume_all_script(enemy->auxScriptID);
                        }
                    }
                }

                enemy = currentEncounter->curEnemy;
                if (!(enemy->flags & ENEMY_FLAG_DO_NOT_KILL)) {
                    encounter = currentEncounter->curEncounter;
                    enemy->aiSuspendTime = 45;
                    playerStatus->blinkTimer = 45;
                    for (j = 0; j < encounter->count; j++) {
                        enemy = encounter->enemy[j];
                        if (enemy == NULL) {
                            continue;
                        }
                        if (enemy->flags & ENEMY_FLAG_DISABLE_AI) {
                            continue;
                        }
                        if (enemy->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) {
                            continue;
                        }
                        enemy->aiSuspendTime = 45;
                        playerStatus->blinkTimer = 45;
                    }
                }

                currentEncounter->battleTriggerCooldown = 45;
                enable_player_input();
                partner_enable_input();
                set_screen_overlay_params_front(OVERLAY_NONE, -1.0f);
                resume_all_group(EVT_GROUP_FLAG_BATTLE);
                gEncounterState = ENCOUNTER_STATE_NEUTRAL;
                EncounterStateChanged = true;
                gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            }
            break;
    }

    for (i = 0; i < currentEncounter->numEncounters; i++) {
        encounter = currentEncounter->encounterList[i];
        if (encounter == NULL) {
            continue;
        }
        for (j = 0; j < encounter->count; j++) {
            enemy = encounter->enemy[j];
            if (enemy == NULL || (enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                continue;
            }

            npc = get_npc_unsafe(enemy->npcID);
            if (enemy->aiSuspendTime != 0) {
                if (enemy->aiSuspendTime & 1) {
                    npc->flags |= NPC_FLAG_SUSPENDED;
                    enemy->flags |= ENEMY_FLAG_SUSPENDED;
                } else {
                    npc->flags &= ~NPC_FLAG_SUSPENDED;
                    enemy->flags &= ~ENEMY_FLAG_SUSPENDED;
                }
            }
        }
    }
}

void draw_encounters_post_battle(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    s32 ret = currentEncounter->fadeOutAccel;

    if (ret != 0) {
        set_screen_overlay_params_front(OVERLAY_SCREEN_COLOR, currentEncounter->fadeOutAmount);
        set_screen_overlay_color(SCREEN_LAYER_FRONT, 0, 0, 0);
    }
}

void update_encounters_conversation(void) {
    EncounterStatus* encounter = &gCurrentEncounter;
    PlayerStatus* playerStatus = &gPlayerStatus;
    Enemy* currentEnemy;
    s32 flag;

    switch (gEncounterSubState) {
        case ENCOUNTER_SUBSTATE_CONVERSATION_INIT:
            currentEnemy = encounter->curEnemy;
            flag = false;

            if (currentEnemy->interactScript != NULL) {
                if (does_script_exist(currentEnemy->interactScriptID)) {
                    flag = true;
                } else {
                    currentEnemy->interactScript = NULL;
                }
            }

            if (currentEnemy->hitScript != NULL) {
                if (does_script_exist(currentEnemy->hitScriptID)) {
                    flag = true;
                } else {
                    currentEnemy->hitScript = NULL;
                }
            }

            if (!flag) {
                gEncounterSubState = ENCOUNTER_SUBSTATE_CONVERSATION_END;
            }
            break;
        case ENCOUNTER_SUBSTATE_CONVERSATION_END:
            resume_all_group(EVT_GROUP_FLAG_INTERACT);

            currentEnemy = encounter->curEnemy;
            if (currentEnemy != NULL && currentEnemy->aiScript != NULL) {
                resume_all_script(currentEnemy->aiScriptID);
            }

            enable_player_input();
            partner_enable_input();

            if (playerStatus->actionState == ACTION_STATE_TALK) {
                set_action_state(ACTION_STATE_IDLE);
            }

            func_800EF3D4(0);
            encounter->hitType = 0;
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
    }
}

void draw_encounters_conversation(void) {
}

bool check_conversation_trigger(void) {
    PlayerStatus* playerStatus = &gPlayerStatus;
    Camera* camera = &gCameras[gCurrentCameraID];
    EncounterStatus* encounterStatus = &gCurrentEncounter;
    f32 npcX, npcY, npcZ;
    f32 angle;
    f32 deltaX, deltaZ;
    Encounter* resultEncounter;
    f32 playerX, playerY, playerZ;
    f32 playerHeight;
    f32 playerRadius;
    f32 length;
    f32 npcHeight;
    f32 npcRadius;
    Encounter* encounter;
    Npc* resultNpc;
    Npc* npc;
    Enemy* resultEnemy;
    Enemy* enemy;
    f32 minLength;
    s32 i, j;
    f32 yaw;

    playerStatus->encounteredNPC = NULL;
    playerStatus->flags &= ~PS_FLAG_HAS_CONVERSATION_NPC;
    playerHeight = playerStatus->colliderHeight;
    playerRadius = playerStatus->colliderDiameter / 2;
    playerX = playerStatus->pos.x;
    playerY = playerStatus->pos.y;
    playerZ = playerStatus->pos.z;

    if (gPartnerStatus.partnerActionState != PARTNER_ACTION_NONE) {
        return false;
    }

    resultEncounter = NULL;
    resultNpc = NULL;
    resultEnemy = NULL;
    minLength = 65535.0f;

    for (i = 0; i < encounterStatus->numEncounters; i++) {
        encounter = encounterStatus->encounterList[i];

        if (encounter == NULL) {
            continue;
        }

        for (j = 0; j < encounter->count; j++) {
            enemy = encounter->enemy[j];

            if (enemy == NULL) {
                continue;
            }

            if (enemy->flags & (ENEMY_FLAG_SUSPENDED | ENEMY_FLAG_DISABLE_AI)) {
                continue;
            }

            if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                continue;
            }

            if ((enemy->flags & ENEMY_FLAG_CANT_INTERACT) || enemy->interactBytecode == NULL) {
                continue;
            }

            npc = get_npc_unsafe(enemy->npcID);

            npcX = npc->pos.x;
            npcY = npc->pos.y;
            npcZ = npc->pos.z;
            deltaX = npcX - playerX;
            deltaZ = npcZ - playerZ;
            npcHeight = npc->collisionHeight;
            npcRadius = npc->collisionDiameter;
            length = sqrtf(SQ(deltaX) + SQ(deltaZ));

            // check cylinder-cylinder overlap
            if ((playerRadius + npcRadius <= length) ||
                (npcY + npcHeight < playerY) ||
                (playerY + playerHeight < npcY)) {
                continue;
            }

            if (clamp_angle(playerStatus->spriteFacingAngle) < 180.0f) {
                angle = clamp_angle(camera->curYaw - 120.0f);
                if (playerStatus->trueAnimation & SPRITE_ID_BACK_FACING) {
                    angle = clamp_angle(angle + 60.0f);
                }
            } else {
                angle = clamp_angle(camera->curYaw + 120.0f);
                if (playerStatus->trueAnimation & SPRITE_ID_BACK_FACING) {
                    angle = clamp_angle(angle - 60.0f);
                }
            }

            yaw = atan2(playerX, playerZ, npcX, npcZ);
            if (fabsf(get_clamped_angle_diff(angle, yaw)) > 90.0f) {
                continue;
            }

            // only allow interact if line of sight exists
            // @bug? flag combination does not make sense
            if (!(enemy->flags & ENEMY_FLAG_RAYCAST_TO_INTERACT) && npc->flags & NPC_FLAG_RAYCAST_TO_INTERACT) {
                f32 x = npcX;
                f32 y = npcY;
                f32 z = npcZ;
                yaw = atan2(npcX, npcZ, playerX, playerZ);

                if (npc_test_move_taller_with_slipping(0, &x, &y, &z, length, yaw, npcHeight, 2.0f * npcRadius)) {
                    continue;
                }
            }

            if (length < minLength) {
                minLength = length;
                resultEncounter = encounter;
                resultNpc = npc;
                resultEnemy = enemy;
            }
        }
    }

    if (!(playerStatus->animFlags & PA_FLAG_8BIT_MARIO) && resultNpc != NULL && !is_picking_up_item()) {
        playerStatus->encounteredNPC = resultNpc;
        playerStatus->flags |= PS_FLAG_HAS_CONVERSATION_NPC;
        if (playerStatus->pressedButtons & BUTTON_A) {
            close_status_bar();
            gCurrentEncounter.hitType = ENCOUNTER_TRIGGER_CONVERSATION;
            resultEnemy->encountered = ENCOUNTER_TRIGGER_CONVERSATION;
            encounterStatus->curEncounter = resultEncounter;
            encounterStatus->curEnemy = resultEnemy;
            encounterStatus->firstStrikeType = FIRST_STRIKE_PLAYER;
            return true;
        }
    }
    return false;
}

void create_encounters(void) {
    EncounterStatus* currentEncounter = &gCurrentEncounter;
    NpcBlueprint sp10;
    NpcBlueprint* bp = &sp10;
    NpcGroup* groupList = (NpcGroup*)(currentEncounter->npcGroupList);
    s32 groupNpcCount;
    s32 mapID = currentEncounter->mapID;

    Npc* newNpc;
    s32 newNpcIndex;
    s32 npcCount;

    NpcSettings* npcSettings;
    NpcData* npcData;
    Enemy* enemy;
    Encounter* encounter;
    Evt* script;

    s32 totalNpcCount;

    s32 cond1;
    s32 cond2;
    s32 i;
    s32 k;
    s32 e;

    switch (gEncounterSubState) {
        case ENCOUNTER_SUBSTATE_CREATE_INIT:
            if (currentEncounter->resetMapEncounterFlags != 1) {
                // check for current map among most recently visited
                for (i = 0; i < ARRAY_COUNT(currentEncounter->recentMaps); i++) {
                    if (currentEncounter->recentMaps[i] == mapID) {
                        break;
                    }
                }
                // current map not found in recent: reset all defeat flags
                if (i >= ARRAY_COUNT(currentEncounter->recentMaps)) {
                    for (k = 0; k < ARRAY_COUNT(currentEncounter->defeatFlags[mapID]); k++) {
                        currentEncounter->defeatFlags[mapID][k] = false;
                    }
                }
                // add current map to recent maps, pushing out the least recent
                for (i = 0; i < ARRAY_COUNT(currentEncounter->recentMaps) - 1; i++) {
                    currentEncounter->recentMaps[i] = currentEncounter->recentMaps[i + 1];
                }
                currentEncounter->recentMaps[i] = mapID;
            }

            e = 0;
            totalNpcCount = 0;
            while (true) {
                if (groupList->npcCount == 0) {
                    break;
                }

                npcData = groupList->npcs;
                groupNpcCount = groupList->npcCount;

                encounter = heap_malloc(sizeof(*encounter));

                currentEncounter->encounterList[e] = encounter;
                ASSERT(encounter != NULL);
                encounter->count = groupNpcCount;
                encounter->battle = groupList->battle;
                encounter->stage = groupList->stage - 1;
                encounter->encounterID = totalNpcCount;
                for (i = 0; i < groupNpcCount; i++) {
                    if (get_defeated(mapID, encounter->encounterID + i)) {
                        npcData++;
                        encounter->enemy[i] = NULL;
                        continue;
                    }

                    enemy = encounter->enemy[i] = heap_malloc(sizeof(*enemy));
                    ASSERT (enemy != NULL);

                    for (k = 0; k < ARRAY_COUNT(enemy->varTable); k++) {
                        enemy->varTable[k] = 0;
                    }
                    enemy->encounterIndex = e;
                    enemy->npcID = npcData->id;
                    npcSettings = enemy->npcSettings = npcData->settings;
                    enemy->drops = &npcData->drops;
                    if ((*(s16*)(&npcData->drops) & 0xFF00) != 0x8000) { //TODO s16?
                        enemy->drops = &DefaultEnemyDrops;
                    }
                    enemy->encountered = 0;
                    if ((s32) npcData->init < EVT_LIMIT) {
                        enemy->initBytecode = npcData->init;
                    } else {
                        enemy->initBytecode = NULL;
                    }
                    enemy->interactBytecode = npcSettings->onInteract;
                    enemy->aiBytecode = npcSettings->ai;
                    enemy->hitBytecode = npcSettings->onHit;
                    enemy->auxBytecode = npcSettings->aux;
                    enemy->defeatBytecode = npcSettings->onDefeat;
                    enemy->initScript = NULL;
                    enemy->interactScript = NULL;
                    enemy->aiScript = NULL;
                    enemy->hitScript = NULL;
                    enemy->auxScript = NULL;
                    enemy->defeatScript = NULL;
                    enemy->interactScriptID = 0;
                    enemy->aiScriptID = 0;
                    enemy->hitScriptID = 0;
                    enemy->auxScriptID = 0;
                    enemy->defeatScriptID = 0;
                    enemy->hitboxIsActive = false;
                    enemy->instigatorValue = 0;
                    enemy->aiDetectFlags = npcData->aiDetectFlags;

                    enemy->aiFlags = npcData->aiFlags;
                    enemy->unk_DC = 0;
                    enemy->aiSuspendTime = 0;
                    enemy->unk_B8 = (EvtScript*)npcSettings->unk_24; // ??
                    enemy->unk_BC = NULL;
                    enemy->unk_C0 = 0;
                    enemy->unk_C4 = 0;

                    enemy->animList = (s32*)&npcData->animations;
                    enemy->territory = &npcData->territory;

                    enemy->flags = npcSettings->flags;
                    enemy->flags |= npcData->flags;
                    enemy->unk_64 = NULL;
                    enemy->tattleMsg = npcData->tattle;
                    if (npcData->initVarCount != 0) {
                        if (npcData->initVarCount == 1) {
                            enemy->varTable[0] = npcData->initVar.value;
                        } else {
                            s32* initialVars = npcData->initVar.array;
                            for (k = 0; k < npcData->initVarCount; k++) {
                                enemy->varTable[k] = *initialVars++;
                            }
                        }
                    }

                    // create the new NPC
                    bp->flags = 0;
                    if (npcSettings->defaultAnim == 0) {
                        bp->initialAnim = enemy->animList[0];
                    } else {
                        bp->initialAnim = npcSettings->defaultAnim;
                    }
                    bp->onUpdate = NULL;
                    bp->onRender = NULL;
                    if (!(enemy->flags & ENEMY_FLAG_USE_PLAYER_SPRITE)) {
                        newNpcIndex = create_standard_npc(bp, npcData->extraAnimations);
                    } else {
                        newNpcIndex = create_peach_npc(bp);
                    }

                    newNpc = get_npc_by_index(newNpcIndex);
                    newNpc->npcID = npcData->id;
                    newNpc->collisionDiameter = npcSettings->radius;
                    newNpc->collisionHeight = npcSettings->height;
                    enemy->spawnPos[0] = newNpc->pos.x = npcData->pos.x;
                    enemy->spawnPos[1] = newNpc->pos.y = npcData->pos.y;
                    enemy->spawnPos[2] = newNpc->pos.z = npcData->pos.z;
                    newNpc->unk_96 = 0;
                    newNpc->planarFlyDist = 0.0f;
                    newNpc->homePos.x = newNpc->pos.x;
                    newNpc->homePos.y = newNpc->pos.y;
                    newNpc->homePos.z = newNpc->pos.z;
                    set_npc_yaw(newNpc, npcData->yaw);
                    enemy->savedNpcYaw = 12345;
                    if (newNpc->collisionDiameter >= 24.0) {
                        newNpc->shadowScale = newNpc->collisionDiameter / 24.0;
                    } else {
                        newNpc->shadowScale = 1.0f;
                    }
                    if (enemy->flags & ENEMY_FLAG_IGNORE_WORLD_COLLISION) {
                        newNpc->flags |= NPC_FLAG_IGNORE_WORLD_COLLISION;
                    }
                    if (enemy->flags & ENEMY_FLAG_IGNORE_PLAYER_COLLISION) {
                        newNpc->flags |= NPC_FLAG_IGNORE_PLAYER_COLLISION;
                    }
                    if (enemy->flags & ENEMY_FLAG_IGNORE_ENTITY_COLLISION) {
                        newNpc->flags |= NPC_FLAG_IGNORE_ENTITY_COLLISION;
                    }
                    if (enemy->flags & ENEMY_FLAG_FLYING) {
                        newNpc->flags |= NPC_FLAG_FLYING;
                    }
                    if (enemy->flags & ENEMY_FLAG_GRAVITY) {
                        newNpc->flags |= NPC_FLAG_GRAVITY;
                    }
                    if (!(enemy->flags & ENEMY_FLAG_PASSIVE)) {
                        newNpc->flags |= NPC_FLAG_IGNORE_PLAYER_COLLISION;
                    }
                    if (enemy->flags & ENEMY_FLAG_HAS_NO_SPRITE) {
                        newNpc->flags |= NPC_FLAG_HAS_NO_SPRITE;
                    }
                    if (enemy->flags & ENEMY_FLAG_NO_SHADOW_RAYCAST) {
                        newNpc->flags |= NPC_FLAG_NO_SHADOW_RAYCAST;
                    }
                    if (enemy->flags & ENEMY_FLAG_USE_INSPECT_ICON) {
                        newNpc->flags |= NPC_FLAG_USE_INSPECT_ICON;
                    }
                    if (enemy->flags & ENEMY_FLAG_RAYCAST_TO_INTERACT) {
                        newNpc->flags |= NPC_FLAG_RAYCAST_TO_INTERACT;
                    }
                    if (enemy->flags & ENEMY_FLAG_DONT_UPDATE_SHADOW_Y) {
                        newNpc->flags |= NPC_FLAG_DONT_UPDATE_SHADOW_Y;
                    }
                    enemy->scriptGroup = EVT_GROUP_HOSTILE_NPC;
                    if (enemy->flags & ENEMY_FLAG_PASSIVE) {
                        enemy->scriptGroup = EVT_GROUP_PASSIVE_NPC;
                    }
                    if (npcSettings->otherAI != NULL) {
                        script = start_script(npcSettings->otherAI, EVT_PRIORITY_A, 0);
                        enemy->aiScript = script;
                        enemy->aiScriptID = script->id;
                        script->owner1.enemy = enemy;
                        script->owner2.npcID = enemy->npcID;
                        script->groupFlags = enemy->scriptGroup;
                    }

                    npcData++;
                }
                groupList++;
                e++;
                totalNpcCount += groupNpcCount;
            }
            currentEncounter->numEncounters = e;
            gEncounterSubState = ENCOUNTER_SUBSTATE_CREATE_RUN_INIT_SCRIPT;
            break;

        case ENCOUNTER_SUBSTATE_CREATE_RUN_INIT_SCRIPT:
            cond2 = false;
            for (e = 0; e < currentEncounter->numEncounters; e++) {
                encounter = currentEncounter->encounterList[e];
                if (encounter == NULL) {
                    continue;
                }
                for (i = 0; i < encounter->count; i++) {
                    enemy = encounter->enemy[i];
                    if (enemy == NULL) {
                        continue;
                    }
                    if (enemy->aiScript != NULL) {
                        if (does_script_exist(enemy->aiScriptID)) {
                            cond2 = true;
                        }
                    }
                }
            }
            if (!cond2) {
                for (e = 0; e < currentEncounter->numEncounters; e++) {
                    encounter = currentEncounter->encounterList[e];
                    if (encounter == NULL) {
                        continue;
                    }
                    for (i = 0; i < encounter->count; i++) {
                        enemy = encounter->enemy[i];
                        if (enemy == NULL) {
                            continue;
                        }
                        if (enemy->initBytecode != NULL) {
                            script = start_script(enemy->initBytecode, EVT_PRIORITY_A, 0);
                            enemy->initScript = script;
                            enemy->initScriptID = script->id;
                            script->owner1.enemy = enemy;
                            script->owner2.npcID = enemy->npcID;
                            script->groupFlags = enemy->scriptGroup;
                        }
                    }
                }
                gEncounterSubState = ENCOUNTER_SUBSTATE_CREATE_RUN_AI;
            }
            break;

        case ENCOUNTER_SUBSTATE_CREATE_RUN_AI:
            cond1 = false;

            for (e = 0; e < currentEncounter->numEncounters; e++) {
                encounter = currentEncounter->encounterList[e];
                if (encounter == NULL) {
                    continue;
                }
                for (i = 0; i < encounter->count; i++) {
                    enemy = encounter->enemy[i];
                    if (enemy == NULL) {
                        continue;
                    }
                    if (enemy->initScript != NULL) {
                        if (does_script_exist(enemy->initScriptID)) {
                            cond1 = true;
                        } else {
                            enemy->initScript = NULL;
                        }
                    }
                }
            }

            if (cond1) {
                break;
            }

            for (e = 0; e < currentEncounter->numEncounters; e++) {
                encounter = currentEncounter->encounterList[e];
                if (encounter == NULL) {
                    continue;
                }
                for (i = 0; i < encounter->count; i++) {
                    enemy = encounter->enemy[i];
                    if (enemy == NULL) {
                        continue;
                    }
                    if (!(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                        if (enemy->aiBytecode != NULL) {
                            script = start_script(enemy->aiBytecode, EVT_PRIORITY_A, 0);
                            enemy->aiScript = script;
                            enemy->aiScriptID = script->id;
                            enemy->unk_C8 = 100;
                            script->owner1.enemy = enemy;
                            script->owner2.npcID = enemy->npcID;
                            script->groupFlags = enemy->scriptGroup;
                        }
                    }
                }
            }

            for (e = 0; e < currentEncounter->numEncounters; e++) {
                encounter = currentEncounter->encounterList[e];
                if (encounter == NULL) {
                    continue;
                }
                for (i = 0; i < encounter->count; i++) {
                    enemy = encounter->enemy[i];
                    if (enemy == NULL) {
                        continue;
                    }
                    if (!(enemy->flags & ENEMY_FLAG_DISABLE_AI)) {
                        if (enemy->auxBytecode != NULL) {
                            script = start_script(enemy->auxBytecode, EVT_PRIORITY_A, 0);
                            enemy->auxScript = script;
                            enemy->auxScriptID = script->id;
                            script->owner1.enemy = enemy;
                            script->owner2.npcID = enemy->npcID;
                            script->groupFlags = enemy->scriptGroup;
                        }
                    }
                }
            }
            resume_all_group(EVT_GROUP_FLAG_BATTLE);
            gEncounterState = ENCOUNTER_STATE_NEUTRAL;
            EncounterStateChanged = true;
            gEncounterSubState = ENCOUNTER_SUBSTATE_NEUTRAL;
            break;
    }
}

void init_encounters_ui(void) {
}

s32 is_starting_conversation(void) {
    s32 ret = gEncounterState == ENCOUNTER_STATE_PRE_BATTLE;

    if (gCurrentEncounter.hitType == ENCOUNTER_TRIGGER_CONVERSATION) {
        ret = true;
    }
    return ret;
}
