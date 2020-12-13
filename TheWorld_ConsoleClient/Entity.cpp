#include "stdafx.h"
#include "Entity.h"

KBEntity::KBEntity(KBEngine::ENTITY_ID eid, SpaceWorld *spaceWorld)
{
	m_eid = eid;
	m_pSpaceWorld = spaceWorld;
	m_bIsPlayer = false;
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
		printf("ENTITY %s - %s - EID: %d, Pos(X/Y/Z): %f / %f / %f, Speed: %f, Dir (Yaw/Pitch/Roll): %f / %f / %f\n", (isPlayer() ? "PLAYER" : "OTHER"), m_name.c_str(), (int)m_eid, m_pos.x, m_pos.y, m_pos.z, m_moveSpeed, m_dir.z, m_dir.y, m_dir.x);
	}
	else
	{
		printf("*** ( Entity %s %d) ******************************************************\n", (isPlayer() ? "PLAYER" : "OTHER"), idx);
		printf("   EntityID                            ==> %d\n", (int)m_eid);
		printf("   Name		                           ==> %s\n", m_name.c_str());
		printf("   SpaceID                             ==> %d\n", (int)m_spaceId);
		printf("   MoveSpeed                           ==> %f\n", m_moveSpeed);
		printf("   Position (X/Y/Z)                    ==> %f / %f / %f\n", m_pos.x, m_pos.y, m_pos.z);
		printf("   Dest. Position (X/Y/Z)              ==> %f / %f / %f\n", m_destPos.x, m_destPos.y, m_destPos.z);
		printf("   Direction (Yaw/Pitch/Roll)          ==> %f / %f / %f\n", m_dir.z, m_dir.y, m_dir.x);
		printf("   Dest. Direction (Yaw/Pitch/Roll)    ==> %f / %f / %f\n", m_destDir.z, m_destDir.y, m_destDir.x);
		printf("   IsOnGround / ModelScale / IsInWorld ==> %d / %f / %d\n", m_bIsOnGround, m_modelScale, m_bIsInWorld);
		printf("*** ( Entity %s %d) ******************************************************\n\n", (isPlayer() ? "PLAYER" : "OTHER"), idx);
	}
}
