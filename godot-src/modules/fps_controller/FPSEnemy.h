/* date = December 17th 2020 1:56 pm */

#ifndef _F_P_S_ENEMY_H
#define _F_P_S_ENEMY_H

#include "scene/3d/node_3d.h"

class CSGBox3D;
class FPSEnemy : public Node3D
{
	GDCLASS(FPSEnemy, Node3D);
	
	public:
	FPSEnemy() {};
	
	void _notification(int p_notification);
	
	void Update(float deltaTime);
	
	void TakeDamage(float hp);
	void Recover();
	
	protected:
	static void _bind_methods();
	
	private:
	float m_hp = 100;
	float m_recoveryTimer = 0;
	float m_timeToRecover = 0.2f;
	bool m_recovering = false;
	
	CSGBox3D* m_model;
	
};

#endif //_F_P_S_ENEMY_H
