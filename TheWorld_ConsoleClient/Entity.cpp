#include "stdafx.h"
#include "Entity.h"

KBEntity::KBEntity(KBEngine::ENTITY_ID eid, SpaceWorld *spaceWorld)
{
	mEID = eid;
	mSPaceWorld = spaceWorld;
}

KBEntity::~KBEntity()
{
}

void KBEntity::dumpStatus(int idx)
{
	printf("*** ( Entity %d) ******************************************************\n", idx);
	printf("   EntityID ==> %d\n", (int)mEID);
	printf("   SpaceID ==> %d\n", (int)mSpaceID);
	printf("   MoveSpeed ==> %f\n", mMoveSpeed);
	printf("   Position (X/Y/Z) ==> %f / %f / %f\n", mPos.x, mPos.y, mPos.z);
	printf("   Dest. Position (X/Y/Z) ==> %f / %f / %f\n", mDestPos.x, mDestPos.y, mDestPos.z);
	printf("   Direction (Yaw/Pitch/Roll) ==> %f / %f / %f\n", mDir.z, mDir.y, mDir.x);
	printf("   Dest. Direction (Yaw/Pitch/Roll) ==> %f / %f / %f\n", mDestDir.z, mDestDir.y, mDestDir.x);
	printf("   IsOnGround / ModelScale / IsInWorld ==> %d / %f / %d\n", mIsOnGround, mModelScale, mInWorld);
	printf("*** ( Entity %d) ******************************************************\n\n", idx);
}
