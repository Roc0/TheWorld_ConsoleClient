#include "stdafx.h"
#include "Entity.h"

KBEntity::KBEntity(KBEngine::ENTITY_ID eid, SpaceWorld *spaceWorld)
{
	mEID = eid;
	mSPaceWorld = spaceWorld;
	mPlayer = false;
}

KBEntity::~KBEntity()
{
}

void KBEntity::attack(KBEntity * receiver, uint32_t skillID, uint32_t damageType, uint32_t damage)
{
}

void KBEntity::recvDamage(KBEntity * attacker, uint32_t skillID, uint32_t damageType, uint32_t damage)
{
}

void KBEntity::dumpStatus(int idx, bool minidump)
{
	if (minidump)
	{
		printf("ENTITY %s - %s - EID: %d, Pos(X/Y/Z): %f / %f / %f, Speed: %f, Dir (Yaw/Pitch/Roll): %f / %f / %f\n", (IsPlayer() ? "PLAYER" : "OTHER"), mName.c_str(), (int)mEID, mPos.x, mPos.y, mPos.z, mMoveSpeed, mDir.z, mDir.y, mDir.x);
	}
	else
	{
		printf("*** ( Entity %s %d) ******************************************************\n", (IsPlayer() ? "PLAYER" : "OTHER"), idx);
		printf("   EntityID                            ==> %d\n", (int)mEID);
		printf("   Name		                           ==> %s\n", mName.c_str());
		printf("   SpaceID                             ==> %d\n", (int)mSpaceID);
		printf("   MoveSpeed                           ==> %f\n", mMoveSpeed);
		printf("   Position (X/Y/Z)                    ==> %f / %f / %f\n", mPos.x, mPos.y, mPos.z);
		printf("   Dest. Position (X/Y/Z)              ==> %f / %f / %f\n", mDestPos.x, mDestPos.y, mDestPos.z);
		printf("   Direction (Yaw/Pitch/Roll)          ==> %f / %f / %f\n", mDir.z, mDir.y, mDir.x);
		printf("   Dest. Direction (Yaw/Pitch/Roll)    ==> %f / %f / %f\n", mDestDir.z, mDestDir.y, mDestDir.x);
		printf("   IsOnGround / ModelScale / IsInWorld ==> %d / %f / %d\n", mIsOnGround, mModelScale, mInWorld);
		printf("*** ( Entity %s %d) ******************************************************\n\n", (IsPlayer() ? "PLAYER" : "OTHER"), idx);
	}
}
