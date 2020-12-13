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
		m_destPos.x = x;
		m_destPos.y = y;
		m_destPos.z = z;
	}

	void setPosition(float x, float y, float z)
	{
		if (x)
			m_pos.x = x;
		if (y)
			m_pos.y = y;
		if (z)
			m_pos.z = z;
	}

	void getPosition(float& x, float& y, float& z)
	{
		x = m_pos.x;
		y = m_pos.y;
		z = m_pos.z;
	}

	void setDestDirection(float yaw, float pitch, float roll)
	{
		m_destDir.x = roll;
		m_destDir.y = pitch;
		m_destDir.z = yaw;
	}

	void setDirection(float yaw, float pitch, float roll)
	{
		m_dir.z = yaw;
		m_dir.y = pitch;
		m_dir.x = roll;
	}

	void setMoveSpeed(float speed)
	{
		m_moveSpeed = speed;
	}

	void setSpaceID(KBEngine::SPACE_ID spaceID)
	{
		m_spaceId = spaceID;
	}

	void setIsOnGround(bool isOnGround)
	{
		m_bIsOnGround = isOnGround;
	}

	void setRes(const std::string & res)
	{
		m_res = res;
	}
	
	void setModelScale(float modelScale)
	{
		m_modelScale = modelScale;
	}

	void setName(const std::string& name)
	{
		m_name = name;
	}
	
	std::string getName(void)
	{
		return m_name;
	}

	void setIsInWorld(bool isInWorld)
	{
		m_bIsInWorld = isInWorld;
	}

	bool getIsInWorld(void)
	{
		return m_bIsInWorld;
	}

	void setModelID(uint32_t modelID)
	{
		m_modelId = modelID;
	}
	
	void setState(uint32_t state)
	{
		m_state = state;
	}

	void setHPMax(uint32_t HPMax)
	{
		m_HPMax = HPMax;
	}

	void setMPMax(uint32_t MPMax)
	{
		m_MPMax = MPMax;
	}

	KBEngine::ENTITY_ID id()const { return m_eid; }

	bool isPlayer() { return m_bIsPlayer;  }

	void attack(KBEntity* receiver, uint32_t skillID, uint32_t damageType, uint32_t damage);
	void recvDamage(KBEntity* attacker, uint32_t skillID, uint32_t damageType, uint32_t damage);

	void dumpStatus(int idx, bool minidump);

protected:
	KBEngine::ENTITY_ID m_eid;				// entityID
	KBEngine::SPACE_ID m_spaceId;
	float m_moveSpeed;
	Vector3 m_destPos, m_pos, m_destDir, m_dir;
	bool m_bIsOnGround;
	std::string m_res;
	SpaceWorld *m_pSpaceWorld;
	float m_modelScale;
	bool m_bIsInWorld;
	bool m_bIsPlayer;
	std::string m_name;

	uint32_t m_modelId;
	int32_t m_state;
	int32_t m_HPMax;
	int32_t m_MPMax;
};
