#include "common.h"
#include "npc.h"

API_CALLABLE(N(IsItemBadge)) {
    Bytecode* args = script->ptrReadPos;
    s32 itemIndex = evt_get_variable(script, *args++);

    script->varTable[0] = false;
    if (gItemTable[itemIndex].typeFlags & ITEM_TYPE_FLAG_BADGE) {
        script->varTable[0] = true;
    }

    return ApiStatus_DONE2;
}
