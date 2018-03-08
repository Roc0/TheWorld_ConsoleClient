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
protected:
	KBEntity(KBEngine::ENTITY_ID eid, SpaceWorld *pSpaceWorld);
public:
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

	void setName(const std::string& name)
	{
		mName = name;
	}
	
	void inWorld(bool inWorld)
	{
		mInWorld = inWorld;
	}

	void setModelID(uint32_t modelID)
	{
		mModelID = modelID;
	}
	
	void setState(uint32_t state)
	{
		mState = state;
	}

	void setHPMax(uint32_t HPMax)
	{
		mHPMax = HPMax;
	}

	void setMPMax(uint32_t MPMax)
	{
		mMPMax = MPMax;
	}

	KBEngine::ENTITY_ID id()const { return mEID; }

	bool IsPlayer() { return mPlayer;  }

	void attack(KBEntity* receiver, uint32_t skillID, uint32_t damageType, uint32_t damage);
	void recvDamage(KBEntity* attacker, uint32_t skillID, uint32_t damageType, uint32_t damage);

	void dumpStatus(int idx, bool minidump);

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
	bool mPlayer;
	std::string mName;

	uint32_t mModelID;
	int32_t mState;
	int32_t mHPMax;
	int32_t mMPMax;
};
