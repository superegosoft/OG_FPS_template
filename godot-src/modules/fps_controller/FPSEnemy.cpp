#include "FPSEnemy.h"

#include "modules/csg/csg_shape.h"
#include "scene/resources/material.h"
#include <thread>

void FPSEnemy::_notification(int p_notification)
{
	switch (p_notification)
	{
		case NOTIFICATION_READY:
		{
			//Only enable update/process/tick in standalone (not in editor)
			//NOTE: "!Engine::get_singleton()->is_editor_hint()" corrolates to NOT in editor
			if (OS::get_singleton()->has_feature("standalone") || !Engine::get_singleton()->is_editor_hint())
			{
				//NOTE: hacky impl, should probably search for a child node that is a model, rather than just having it be specified by index, this is quicker though
				m_model = (CSGBox3D*)get_child(0)->get_child(1);
				set_physics_process(true);
			}
			
		}break;
		case NOTIFICATION_PROCESS:
		{
		}break;
		case NOTIFICATION_PHYSICS_PROCESS:
		{
			
			Update(get_process_delta_time());
		}break;
	}
}

void FPSEnemy::Update(float deltaTime)
{
	if (m_recovering)
	{
		m_recoveryTimer += get_physics_process_delta_time();
		if (m_recoveryTimer > m_timeToRecover)
			Recover();
	}
}

void FPSEnemy::TakeDamage(float hp)
{
	print_line("ENEMY HIT");
	if(m_model)
	{
		((Ref<BaseMaterial3D>)m_model->get_material())->set_albedo(Color(1,1,1,1));
	}
	m_recoveryTimer = 0;
	m_recovering = true;
}

//not 100% sure how to make some kind of async function safely here without too much work outside of the engine, so this is a thing
void FPSEnemy::Recover()
{
	if(m_model)
	{
		//hard set
		((Ref<BaseMaterial3D>)m_model->get_material())->set_albedo(Color(1,0,0,1));
	}
	m_recovering = false;
}

//method required to bind events (make them known to the engine)
void FPSEnemy::_bind_methods()
{
	
	ClassDB::bind_method(D_METHOD("_notification", "p_notification"), &FPSEnemy::_notification);
}
