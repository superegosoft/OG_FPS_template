#include "FPSController.h"

#include "core/input/input.h"
#include "core/config/engine.h"
#include "core/math/math_funcs.h"
#include "scene/main/viewport.h"
#include "servers/physics_server_3d.h"
#include "core/io/resource_loader.h"
#include "scene/gui/texture_rect.h"
#include "FPSEnemy.h"

#include <algorithm>

//definitions for forward / right vectors for this node3D.
//typing them out each time is a pain and can make code cluttered
//NOTE: basis.get_axis(2) == forward vector, basis.get_axis(0) == right vector
#define FORWARD() get_global_transform().basis.get_axis(2)
#define RIGHT() get_global_transform().basis.get_axis(0)

FPSController::FPSController()
{
}

void FPSController::_notification(int p_notification)
{
	switch (p_notification)
	{
		case NOTIFICATION_READY:
		{
			p_input = Input::get_singleton();
			Ref<World3D> w3d = get_world_3d();
			p_physics = PhysicsServer3D::get_singleton()->space_get_direct_state(w3d->get_space());
			//Only enable update/process/tick in standalone (not in editor)
			//NOTE: "!Engine::get_singleton()->is_editor_hint()" corrolates to NOT in editor
			if (OS::get_singleton()->has_feature("standalone") || !Engine::get_singleton()->is_editor_hint())
			{
				//everything happens on the physics process / fixed update right now
				set_process(true);
				set_physics_process(true);
				//Hide the cursor
				p_input->set_mouse_mode(Input::MouseMode::MOUSE_MODE_HIDDEN);
				p_input->warp_mouse_position(get_viewport()->get_visible_rect().size * 0.5f);
				//load frames for anim
				m_idleFrame = ResourceLoader::load(IDLE_ANIM_PATH);
				m_fireFrame = ResourceLoader::load(FIRE_ANIM_PATH);
				m_kickbackFrame =ResourceLoader::load(KICKBACK_ANIM_PATH);
				m_recoverFrame = ResourceLoader::load(RECOVER_ANIM_PATH);
			}
			p_camera = (Node3D*)get_child(0);
			p_handSprite = (TextureRect*)get_child(1);
			
		}break;
		case NOTIFICATION_PROCESS:
		{
			Animate(get_process_delta_time());
		}break;
		case NOTIFICATION_PHYSICS_PROCESS:
		{
			Update(get_process_delta_time());
		}break;
	}
}

void FPSController::UpdateInput()
{
	//Getting direction for movement
	m_direction = Vector3(-p_input->get_axis("left","right"), 0, p_input->get_axis("back","forward"));
	m_direction = m_direction.normalized();
	
	//getting mouse delta using distance travelled from the center of the screen this update
	Vector2 screenCenter = get_viewport()->get_visible_rect().size * 0.5f;
	m_mousePos = p_input->get_mouse_position();
	m_mouseDelta = (screenCenter - m_mousePos) * m_sensitivity;
	
	//reset mouse pos to center of the screen
	p_input->warp_mouse_position(screenCenter);
	
	//firing
	if (p_input->is_action_just_pressed("fire"))
	{
		//rate of fire tied to animation at the moment, this is quite hacky, shouldn't be implemented this way but I wanted to get animations in quick
		if (!m_animating)
			Fire();
	}
}

//Called every fixed amount of time on the physics_process step
void FPSController::Update(float deltaTime)
{
	UpdateInput();
	
	//ROTATING CAMERA
	//Yaw rotation, actually rotate the object
	rotate_y(m_mouseDelta.x * get_physics_process_delta_time());
	
	//Pitch rotation, rotate just the camera so it doesn't affect our forward
	float cameraPitch = p_camera->get_transform().basis.get_euler().x;
	float pitchDelta = m_mouseDelta.y * get_physics_process_delta_time();
	
	//clamping to restrict camera pitch, cannot hard set pitch easily by component,
	//and since we want to leave the door open to camera movement effects (such as screen shake) 
	//which is why we clamp the final value between -1.5 and 1.5 radians and then subtract it from the initial value
	
	float finalPitchChange = cameraPitch - CLAMP(cameraPitch + pitchDelta, -1.5, 1.5);
	
	//apply camera rotation
	p_camera->rotate_x(finalPitchChange);
	
	//MOVEMENT
	Vector3 horizontalVelocity = m_velocity;
	horizontalVelocity.y = 0; //removing y vel from horizontal vel
	if (m_direction != Vector3(0,0,0))
	{
		horizontalVelocity = m_velocity + ((m_direction.z * FORWARD()) 
										   + (m_direction.x * RIGHT())) 
			* m_movespeed;
		
		//clamp horizontal vector to max movespeed
		horizontalVelocity = horizontalVelocity.normalized() * std::min(horizontalVelocity.length(), m_maxMoveSpeed);
		//then multiply by delta time, makes it easier to judge how long it'll take to reach max movespeed given movespeed 
		horizontalVelocity *= get_physics_process_delta_time();
	}
	
	
	//need to make sure velocity is non zero before casting rays
	if (horizontalVelocity.length() > MIN_MOVE_SPEED)
	{
		//NOTE: structure for containing data from raycast
		PhysicsDirectSpaceState3D::RayResult rayHitResult;
		
		//FORWARD RAYCAST
		//all this to say, raycast in the direction we're moving in + a predefined player radius in that direction
		
		//do several forward raycasts instead of just one
		//set start point, bottom left
		Vector3 origin = get_global_transform().origin - (RIGHT() * m_playerRadius * 0.5f + Vector3(0, m_playerHeight * 0.5f,0));
		//calculate the vector spacing inbetween each ray using player radius / height and the number of rays specified in the header as const ints
		Vector3 horizontalSpacing = RIGHT() * (m_playerRadius / NUM_HORIZONTAL_RAYS);
		Vector3 verticalSpacing = Vector3(0, m_playerHeight / NUM_VERTICAL_RAYS, 0);
		
		//cast all the rays
		for(int x = 0; x < NUM_HORIZONTAL_RAYS; ++x)
		{
			for (int y = 0; y < NUM_VERTICAL_RAYS; ++y)
			{
				if (y == 0)
					origin += Vector3(0, GROUND_PADDING,0);
				if (p_physics->intersect_ray(origin, origin + horizontalVelocity + m_playerRadius * horizontalVelocity.normalized(), rayHitResult))
				{
					//if we hit a ray we don't want to just simply stop the player in their tracks
					//getting stuck on environments feels bad, instead slide them along it.
					//here we calculate the vector going along the wall, then set the players velocity to that vector * speed * the angle that they're coming in on
					Vector3 collisionNormal = rayHitResult.normal;
					Vector3 wallPerp = collisionNormal.cross(Vector3(0,-1,0));
					Vector3 nVelocity = horizontalVelocity.normalized();
					float angle = Math::rad2deg(collisionNormal.angle_to(-nVelocity));
					
					if (nVelocity.x * -collisionNormal.z > nVelocity.z * -collisionNormal.x)
					{
						wallPerp *= -1;
					}
					
					//making a modifier based on the shallowness of the angle when walking at a wall, shallow angles will hardly move the player at all, greater angles will move the player closer to their max move speed
					float velocityMod = angle / 90;
					
					//only affect the x and z values here we want to make jumping / vertical movement seperate
					horizontalVelocity.x = (wallPerp.x * m_movespeed * get_physics_process_delta_time()) * velocityMod;
					horizontalVelocity.z = (wallPerp.z * m_movespeed * get_physics_process_delta_time()) * velocityMod;
				}
				//increment up the players bounds
				origin += verticalSpacing;
			}
			//reset the y value for another x increment
			origin.y = get_global_transform().origin.y - m_playerHeight * 0.5f;
			//increment across the player
			origin += horizontalSpacing;
		}
		//reduce speed
		horizontalVelocity.x *= 0.5;
		horizontalVelocity.z *= 0.5;
	}
	else
	{
		horizontalVelocity = Vector3(0,0,0);
	}
	
	//FALLING 
	Vector3 verticalVelocity;
	verticalVelocity.y = m_velocity.y;
	//and collision check..
	//casting just one ray down for now
	
	PhysicsDirectSpaceState3D::RayResult rayHitResult;
	//get whichever is bigger and use that for distance. Should prevent the player falling through the floor if they're moving 
	float rayDistance = m_playerHeight * 0.5 > ABS(verticalVelocity.y) ? m_playerHeight * 0.5 : ABS(verticalVelocity.y);
	
	if (p_physics->intersect_ray(get_global_transform().origin, get_global_transform().origin - Vector3(0, rayDistance ,0), rayHitResult))
	{
		verticalVelocity.y = 0;
		//place the player at half their hight above the point hit, stops them from falling through the floor if the ray catches them too late
		//NOTE: for some reason here if we use global_translate it produces unexpected results but if we go the long way round, it works fine
		translate(to_local(Vector3(get_global_transform().origin.x, rayHitResult.position.y + (m_playerHeight * 0.49), get_global_transform().origin.z)));
		m_grounded = true;
		m_jumpsTaken = 0;
	}
	else
	{
		verticalVelocity.y -= m_playerGravity * get_physics_process_delta_time();
		m_grounded = false;
	}
	
	
	//JUMPING
	//have to check if we want to jump after the ground check here
	if(p_input->is_action_just_pressed("jump") && (m_grounded || m_jumpsTaken < m_maxJumps))
	{
		verticalVelocity.y = m_jumpForce;
		m_jumpsTaken++;
	}
	
	//setting velocity, adding both x/z and y compontents
	m_velocity = horizontalVelocity + verticalVelocity;
	//finally, actually update the position
	set_translation(get_translation() + m_velocity);
}

//hacky animation, ideally we'd want some kind of state machine to handle this
void FPSController::Animate(float deltaTime)
{
	if(m_animating)
	{
		m_animationTime += deltaTime;
		if (m_animationTime > m_fireAnimLength)
		{
			m_animating = false;
			m_animationTime = 0;
			//set gun to idle
			p_handSprite->set_texture(m_idleFrame);
			return;
		}
		
		float timePerFrame = m_fireAnimLength / NUM_FIRE_FRAMES;
		
		if (m_animationTime <= timePerFrame)
		{
			//first frame;
			p_handSprite->set_texture(m_fireFrame);
		}
		
		else if (m_animationTime > timePerFrame && m_animationTime <= timePerFrame * 2)
		{
			//second frame
			p_handSprite->set_texture(m_kickbackFrame);
		}
		
		else
		{
			//last frame
			p_handSprite->set_texture(m_recoverFrame);
		}
		
	}
}

//kinda hacky fire implmentation
void FPSController::Fire()
{
	//hacky start animating fire animation
	m_animating = true;
	//cast a ray, if that hits...
	PhysicsDirectSpaceState3D::RayResult rayHitResult;
	if (p_physics->intersect_ray(get_global_transform().origin, get_global_transform().origin + -p_camera->get_global_transform().basis.get_axis(2) * m_playerRange, rayHitResult))
	{
		List<PropertyInfo> propertyList;
		//get root element
		Node* root = (Node*)rayHitResult.collider;
		//iterate through parents until you get one that is of type FPSEnemy...
		while (true)
		{
			//NOTE: currently just searches for a class of that name.
			//TODO: need to search for a type that inherits from that class instead
			if (root->get_class_name() == "FPSEnemy")
			{
				//have that enemy take damage
				FPSEnemy* enemy = (FPSEnemy*)root;
				enemy->TakeDamage(10);
				return;
			}
			if (!root->get_parent())
				return;
			root = root->get_parent();
		}
	}
}

//method required to bind events (make them known to the engine)
void FPSController::_bind_methods()
{
	
	ClassDB::bind_method(D_METHOD("_notification", "p_notification"), &FPSController::_notification);
}