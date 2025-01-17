/*
 * File: z_obj_etcetera.c
 * Overlay: ovl_Obj_Etcetera
 * Description: Deku Flower
 */

#include "z_obj_etcetera.h"

#define FLAGS 0x00000010

#define THIS ((ObjEtcetera*)thisx)

void ObjEtcetera_Init(Actor* thisx, GlobalContext* globalCtx);
void ObjEtcetera_Destroy(Actor* thisx, GlobalContext* globalCtx);
void ObjEtcetera_Update(Actor* thisx, GlobalContext* globalCtx);

void ObjEtcetera_PlaySmallFlutterAnimation(ObjEtcetera* this, GlobalContext* globalCtx);
void ObjEtcetera_DoIntenseOscillation(ObjEtcetera* this, GlobalContext* globalCtx);
void ObjEtcetera_Setup(ObjEtcetera* this, GlobalContext* globalCtx);
void ObjEtcetera_DrawIdle(Actor* thisx, GlobalContext* globalCtx);
void ObjEtcetera_DrawAnimated(Actor* thisx, GlobalContext* globalCtx);

const ActorInit Obj_Etcetera_InitVars = {
    ACTOR_OBJ_ETCETERA,
    ACTORCAT_BG,
    FLAGS,
    GAMEPLAY_KEEP,
    sizeof(ObjEtcetera),
    (ActorFunc)ObjEtcetera_Init,
    (ActorFunc)ObjEtcetera_Destroy,
    (ActorFunc)ObjEtcetera_Update,
    (ActorFunc)NULL,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x00000000, 0x00, 0x00 },
        { 0x01000202, 0x00, 0x00 },
        TOUCH_NONE | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_NONE,
    },
    { 20, 14, 0, { 0, 0, 0 } },
};

extern ColliderCylinderInit D_80A7C790;

static s16 objectIds[] = {
    GAMEPLAY_KEEP,
    GAMEPLAY_KEEP,
    GAMEPLAY_KEEP,
    GAMEPLAY_KEEP,
};

/**
 * Most interactions with a Deku Flower cause it to slightly oscillate on the X and Z axes.
 * When these small oscillations happen, the game determines how to scale the appropriate
 * axes by using this table.
 */
static f32 oscillationTable[] = {
    -1.0, -1.0, -1.0, -0.7, 0.0, 0.7, 1.0, 0.7, 0.0, -0.7, -1.0, -0.7, 0.0, 0.7, 1.0, 0.7, 0.0, -0.7,
};

void ObjEtcetera_Init(Actor* thisx, GlobalContext* globalCtx) {
    s32 pad;
    ObjEtcetera* this = THIS;
    s32 objectIndex;
    s32 type = DEKU_FLOWER_TYPE(&this->dyna.actor);
    s32 floorBgId;
    Vec3f pos;

    if ((type < DEKU_FLOWER_TYPE_PINK) || (type >= DEKU_FLOWER_TYPE_MAX)) {
        type = DEKU_FLOWER_TYPE_PINK;
    }
    objectIndex = Object_GetIndex(&globalCtx->objectCtx, objectIds[type]);
    if (objectIndex >= 0) {
        this->objIndex = objectIndex;
    }
    pos.x = this->dyna.actor.world.pos.x;
    pos.y = this->dyna.actor.world.pos.y + 10.0f;
    pos.z = this->dyna.actor.world.pos.z;
    func_800C411C(&globalCtx->colCtx, &this->dyna.actor.floorPoly, &floorBgId, &this->dyna.actor, &pos);
    this->dyna.actor.floorBgId = floorBgId;
    Collider_InitAndSetCylinder(globalCtx, &this->collider, &this->dyna.actor, &sCylinderInit);
    Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    this->actionFunc = ObjEtcetera_Setup;
    Actor_SetScale(&this->dyna.actor, 0.01f);
    this->dyna.actor.scale.y = 0.02f;
    this->burrowFlag = 0;
}

void ObjEtcetera_Destroy(Actor* thisx, GlobalContext* globalCtx) {
    ObjEtcetera* this = THIS;

    BgCheck_RemoveActorMesh(globalCtx, &globalCtx->colCtx.dyna, this->dyna.bgId);
    Collider_DestroyCylinder(globalCtx, &this->collider);
}

void ObjEtcetera_DoNormalOscillation(ObjEtcetera* this, GlobalContext* globalCtx) {
    if (this->oscillationTimer > 0) {
        s32 requiredScopeTemp;

        Actor_SetScale(&this->dyna.actor,
                       (oscillationTable[globalCtx->gameplayFrames % 18] * (0.0001f * this->oscillationTimer)) + 0.01f);
        this->dyna.actor.scale.y = 0.02f;
        this->oscillationTimer--;
    } else {
        Actor_SetScale(&this->dyna.actor, 0.01f);
        this->dyna.actor.scale.y = 0.02f;
    }
}

void ObjEtcetera_StartSmallFlutterAnimation(ObjEtcetera* this) {
    Animation_Change(&this->skelAnime, &D_040117A8, 1.0f, 0.0f, Animation_GetLastFrame(&D_040117A8), 2, 0.0f);
    this->dyna.actor.draw = ObjEtcetera_DrawAnimated;
    this->actionFunc = ObjEtcetera_PlaySmallFlutterAnimation;
}

void ObjEtcetera_Idle(ObjEtcetera* this, GlobalContext* globalCtx) {
    s16 minOscillationTimer;
    Player* player = GET_PLAYER(globalCtx);

    if ((player->stateFlags3 & 0x200) && (this->dyna.actor.xzDistToPlayer < 20.0f)) {
        // Player is launching out of the Deku Flower
        Animation_Change(&this->skelAnime, &D_0400EB7C, 1.0f, 0.0f, Animation_GetLastFrame(&D_0400EB7C), 2, 0.0f);
        this->dyna.actor.draw = ObjEtcetera_DrawAnimated;
        this->actionFunc = ObjEtcetera_DoIntenseOscillation;
        Actor_SetScale(&this->dyna.actor, 0.01f);
        this->dyna.actor.scale.y = 0.02f;
        this->intenseOscillationScale = 0.003f;
        this->oscillationTimer = 30;
        this->burrowFlag &= ~1;
    } else if ((player->stateFlags3 & 0x2000) && (this->dyna.actor.xzDistToPlayer < 30.0f) &&
               (this->dyna.actor.playerHeightRel > 0.0f)) {
        // Player is hovering above the Deku Flower
        minOscillationTimer = 10 - (s32)(this->dyna.actor.playerHeightRel * 0.05f);
        if (this->oscillationTimer < minOscillationTimer) {
            this->oscillationTimer = minOscillationTimer;
        }
    } else {
        if (func_800CAF70(&this->dyna)) {
            if (!(this->burrowFlag & 1)) {
                // Player is walking onto the Deku Flower, or falling on it from a height
                this->oscillationTimer = 10;
                ObjEtcetera_StartSmallFlutterAnimation(this);
            } else if ((player->actor.speedXZ > 0.1f) || ((player->unk_ABC < 0.0f) && !(player->stateFlags3 & 0x100))) {
                // Player is walking on top of the Deku Flower, is at the very start of burrowing, or is at the very
                // start of launching
                this->oscillationTimer = 10;
            }
            this->burrowFlag |= 1;
        } else {
            if (this->burrowFlag & 1) {
                // Player is walking off the Deku Flower
                this->oscillationTimer = 10;
                ObjEtcetera_StartSmallFlutterAnimation(this);
            }
            this->burrowFlag &= ~1;
        }
    }
    if ((this->collider.base.acFlags & AC_HIT)) {
        this->oscillationTimer = 10;
        ObjEtcetera_StartSmallFlutterAnimation(this);
    }
    ObjEtcetera_DoNormalOscillation(this, globalCtx);
}

void ObjEtcetera_PlaySmallFlutterAnimation(ObjEtcetera* this, GlobalContext* globalCtx) {
    if (func_800CAF70(&this->dyna)) {
        this->burrowFlag |= 1;
    } else {
        this->burrowFlag &= ~1;
    }
    if (SkelAnime_Update(&this->skelAnime)) {
        this->dyna.actor.draw = ObjEtcetera_DrawIdle;
        this->actionFunc = ObjEtcetera_Idle;
    }
    ObjEtcetera_DoNormalOscillation(this, globalCtx);
}

void ObjEtcetera_DoIntenseOscillation(ObjEtcetera* this, GlobalContext* globalCtx) {
    // In order to match, we are seemingly required to access scale.x at one point
    // without using this. We can create a thisx or dyna pointer to achieve that, but
    // it's more likely they used dyna given that func_800CAF70 takes a DynaPolyActor.
    DynaPolyActor* dyna = &this->dyna;
    f32 scaleTemp;

    if (func_800CAF70(dyna)) {
        this->burrowFlag |= 1;
    } else {
        this->burrowFlag &= ~1;
    }
    SkelAnime_Update(&this->skelAnime);
    if (this->oscillationTimer > 0) {
        this->oscillationTimer--;
    } else {
        this->dyna.actor.draw = ObjEtcetera_DrawIdle;
        this->actionFunc = ObjEtcetera_Idle;
        Actor_SetScale(&this->dyna.actor, 0.01f);
        this->dyna.actor.scale.y = 0.02f;
        this->oscillationTimer = 0;
        this->intenseOscillationScale = 0.0f;
        return;
    }
    this->intenseOscillationScale *= 0.8f;
    this->intenseOscillationScale -= (this->dyna.actor.scale.x - 0.01f) * 0.4f;
    scaleTemp = dyna->actor.scale.x + this->intenseOscillationScale;
    Actor_SetScale(&this->dyna.actor, scaleTemp);
    this->dyna.actor.scale.y = 2.0f * scaleTemp;
}

void ObjEtcetera_Setup(ObjEtcetera* this, GlobalContext* globalCtx) {
    CollisionHeader* colHeader = NULL;
    s32 type;
    CollisionHeader* collisionHeaders[] = { &D_0400E710, &D_0400E710, &D_040118D8, &D_040118D8 };
    s32 pad;
    CollisionHeader* thisCollisionHeader;

    type = DEKU_FLOWER_TYPE(&this->dyna.actor);
    if ((type < DEKU_FLOWER_TYPE_PINK) || (type >= DEKU_FLOWER_TYPE_MAX)) {
        type = DEKU_FLOWER_TYPE_PINK;
    }

    if (Object_IsLoaded(&globalCtx->objectCtx, this->objIndex)) {
        this->dyna.actor.objBankIndex = this->objIndex;
        Actor_SetObjectSegment(globalCtx, &this->dyna.actor);
        BcCheck3_BgActorInit(&this->dyna, 1);
        thisCollisionHeader = collisionHeaders[type];
        if (thisCollisionHeader != 0) {
            BgCheck_RelocateMeshHeader(thisCollisionHeader, &colHeader);
        }
        this->dyna.bgId = BgCheck_AddActorMesh(globalCtx, &globalCtx->colCtx.dyna, &this->dyna, colHeader);

        type = DEKU_FLOWER_TYPE(&this->dyna.actor);
        switch (type) {
            case DEKU_FLOWER_TYPE_PINK:
            case DEKU_FLOWER_TYPE_PINK_SPAWNED_FROM_MAD_SCRUB:
                SkelAnime_Init(globalCtx, &this->skelAnime, &D_04011518, &D_0400EB7C, this->jointTable,
                               this->morphTable, 11);
                this->dList = &D_0400ED80;
                break;
            case DEKU_FLOWER_TYPE_GOLD:
            case DEKU_FLOWER_TYPE_GOLD_SPAWNED_FROM_MAD_SCRUB:
                this->dList = &D_04011BD0;
                SkelAnime_Init(globalCtx, &this->skelAnime, &D_040127E8, &D_0400EB7C, this->jointTable,
                               this->morphTable, 11);
                this->collider.dim.height = 20;
                break;
        }

        type = DEKU_FLOWER_TYPE(&this->dyna.actor);
        switch (type) {
            case DEKU_FLOWER_TYPE_PINK:
            case DEKU_FLOWER_TYPE_GOLD:
                this->dyna.actor.draw = ObjEtcetera_DrawIdle;
                this->actionFunc = ObjEtcetera_Idle;
                Actor_SetScale(&this->dyna.actor, 0.01f);
                this->dyna.actor.scale.y = 0.02f;
                this->dyna.actor.focus.pos.y = this->dyna.actor.home.pos.y + 10.0f;
                this->dyna.actor.targetMode = 3;
                break;
            case DEKU_FLOWER_TYPE_PINK_SPAWNED_FROM_MAD_SCRUB:
            case DEKU_FLOWER_TYPE_GOLD_SPAWNED_FROM_MAD_SCRUB:
                Animation_Change(&this->skelAnime, &D_0400EB7C, 1.0f, 0.0f, Animation_GetLastFrame(&D_0400EB7C), 2,
                                 0.0f);
                this->dyna.actor.draw = ObjEtcetera_DrawAnimated;
                this->actionFunc = ObjEtcetera_DoIntenseOscillation;
                Actor_SetScale(&this->dyna.actor, 0.0f);
                this->oscillationTimer = 30;
                this->intenseOscillationScale = 0.0f;
                this->dyna.actor.focus.pos.y = this->dyna.actor.home.pos.y + 10.0f;
                this->dyna.actor.targetMode = 3;
                break;
        }
    }
}

void ObjEtcetera_Update(Actor* thisx, GlobalContext* globalCtx) {
    ObjEtcetera* this = THIS;
    CollisionPoly* floorPoly;
    u8 floorBgId = this->dyna.actor.floorBgId;

    if (floorBgId == BGCHECK_SCENE) {
        floorPoly = this->dyna.actor.floorPoly;
        if (floorPoly != NULL && this->burrowFlag & 1) {
            func_800FAAB4(globalCtx, func_800C9C9C(&globalCtx->colCtx, floorPoly, floorBgId));
        }
    }
    this->actionFunc(this, globalCtx);
    CollisionCheck_SetAC(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
}

void ObjEtcetera_DrawIdle(Actor* thisx, GlobalContext* globalCtx) {
    ObjEtcetera* this = THIS;

    OPEN_DISPS(globalCtx->state.gfxCtx);

    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    func_8012C28C(globalCtx->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, this->dList);

    CLOSE_DISPS(globalCtx->state.gfxCtx);
}

void ObjEtcetera_DrawAnimated(Actor* thisx, GlobalContext* globalCtx) {
    ObjEtcetera* this = THIS;

    func_8012C5B0(globalCtx->state.gfxCtx);
    SkelAnime_DrawOpa(globalCtx, this->skelAnime.skeleton, this->skelAnime.jointTable, NULL, NULL, &this->dyna.actor);
}
