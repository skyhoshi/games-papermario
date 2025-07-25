#include "common.h"

#include "world/common/todo/StashVars.inc.c"
#include "world/common/todo/GetItemName.inc.c"
#include "world/common/todo/SomeItemEntityFunc.inc.c"
#include "world/common/todo/IsItemBadge.inc.c"

s32** N(varStash) = NULL;

EvtScript N(EVS_Chest_ShowGotItem) = {
    SetGroup(EVT_GROUP_NEVER_PAUSE)
    Call(SetTimeFreezeMode, TIME_FREEZE_FULL)
    Wait(40)
    Call(ShowGotItem, LVar0, false, 0)
    Call(SetTimeFreezeMode, TIME_FREEZE_NONE)
    Return
    Return
    End
};

EvtScript N(EVS_Chest_GetItem) = {
    Call(DisablePlayerInput, true)
    Set(LVar0, LVarA)
    IfNe(LVarA, ITEM_NONE)
        ExecWait(N(EVS_Chest_ShowGotItem))
    EndIf
    Switch(LVarB)
        CaseEq(ITEM_TYPE_CONSUMABLE)
            Call(AddItem, LVarA, LVar0)
        CaseEq(ITEM_TYPE_KEY)
            Call(AddKeyItem, LVarA)
        CaseEq(ITEM_TYPE_BADGE)
            Call(AddBadge, LVarA, LVar0)
    EndSwitch
    Wait(15)
    Call(DisablePlayerInput, false)
    Return
    End
};

#define EVT_OPEN_CHEST_CONSUMABLE(item, flag) \
    { \
        Set(LVarA, item) \
        Set(LVarB, ITEM_TYPE_CONSUMABLE) \
        Set(flag, true) \
        ExecWait(N(EVS_Chest_GetItem)) \
        Return \
        End \
    }

#define EVT_OPEN_CHEST_KEY(item, flag) \
    { \
        Set(LVarA, item) \
        Set(LVarB, ITEM_TYPE_KEY) \
        Set(flag, true) \
        ExecWait(N(EVS_Chest_GetItem)) \
        Return \
        End \
    }

#define EVT_OPEN_CHEST_BADGE(item, flag) \
    { \
        Set(LVarA, item) \
        Set(LVarB, ITEM_TYPE_BADGE) \
        Set(flag, true) \
        ExecWait(N(EVS_Chest_GetItem)) \
        Return \
        End \
    }
