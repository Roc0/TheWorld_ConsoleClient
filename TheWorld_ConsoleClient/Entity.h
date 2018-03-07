#pragma once
#include "SpaceWorld.h"

#include "TheWorld_ClientDll.h"

class Vector3
{
public:
	float x;
	float y;
	float z;
};

class KBEntity
{
public:
	KBEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld);
	virtual ~KBEntity();

	void setDestPosition(float x, float y, float z)
	{
		mDestPos.x = x;
		mDestPos.y = y;
		mDestPos.z = z;
	}

	void setPosition(float x, float y, float z)
	{
		mPos.x = x;
		mPos.y = y;
		mPos.z = z;
	}

	void setDestDirection(float yaw, float pitch, float roll)
	{
		mDestDir.x = roll;
		mDestDir.y = pitch;
		mDestDir.z = yaw;
	}

	void setDirection(float yaw, float pitch, float roll)
	{
		mDir.z = yaw;
		mDir.y = pitch;
		mDir.x = roll;
	}

	void setMoveSpeed(float speed)
	{
		mMoveSpeed = speed;
	}

	void setSpaceID(KBEngine::SPACE_ID spaceID)
	{
		mSpaceID = spaceID;
	}

	void setIsOnGround(bool isOnGround)
	{
		mIsOnGround = isOnGround;
	}

	void setRes(const std::string & res)
	{
		mRes = res;
	}
	
	void setModelScale(float modelScale)
	{
		mModelScale = modelScale;
	}
	
	void inWorld(bool inWorld)
	{
		mInWorld = inWorld;
	}
	
	KBEngine::ENTITY_ID id()const { return mEID; }

	void dumpStatus(int idx);

protected:
	KBEngine::ENTITY_ID mEID;				// entityID
	KBEngine::SPACE_ID mSpaceID;
	float mMoveSpeed;
	Vector3 mDestPos, mPos, mDestDir, mDir;
	bool mIsOnGround;
	std::string mRes;
	SpaceWorld *mSPaceWorld;
	float mModelScale;
	bool mInWorld;
};
