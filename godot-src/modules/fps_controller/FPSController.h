/* date = December 10th 2020 4:09 pm */

#ifndef _F_P_S_CONTROLLER_H
#define _F_P_S_CONTROLLER_H
#include "scene/3d/node_3d.h"
#include "scene/resources/texture.h"

class Input;
class PhysicsDirectSpaceState3D;
class TextureRect;
class FPSController : public Node3D
{
	GDCLASS(FPSController, Node3D);
	
	public:
	FPSController();
	
	void _notification(int p_notification);
	void _init();
	
	void UpdateInput();
	void Update(float deltaTime);
	
	void Animate(float deltaTime);
	void Fire();
	
	protected:
	
	static void _bind_methods();
	
	private:
	//singleton pointers
	Input* p_input;
	PhysicsDirectSpaceState3D* p_physics;
	
	//player properties
	float m_playerRadius = 2.0f;
	float m_playerHeight = 4.f;
	float m_playerRange = 100000;
	float m_playerGravity = 2.f;
	//CAMERA
	Point2 m_mousePos;
	Vector2 m_mouseDelta;
	float m_sensitivity = 0.3f;
	//MOVEMENT
	bool m_collided = false;
	float m_movespeed = 50.0f;
	float m_maxMoveSpeed = 46.0f;
	float m_jumpForce = 0.65f;
	bool m_grounded = true;
	int m_jumpsTaken = 0;
	int m_maxJumps = 2;
	Vector3 m_velocity;
	Vector3 m_direction;
	Node3D* p_camera;
	//COLLISIONS
	const int NUM_VERTICAL_RAYS = 3;
	const int NUM_HORIZONTAL_RAYS = 3;
	const float GROUND_PADDING = 0.1f;
	//ANIMATION
	float m_animationTime = 0;
	float m_fireAnimLength = 0.5;
	bool m_animating = false;
	//GUI HAND / GUN 
	TextureRect* p_handSprite;
	//ANIMATION FRAMES
	const int NUM_FIRE_FRAMES = 3;
	Ref<Texture2D> m_idleFrame;
	Ref<Texture2D> m_fireFrame;
	Ref<Texture2D> m_kickbackFrame;
	Ref<Texture2D> m_recoverFrame;
	//ANIMATION FRAME PATHS
	const char* IDLE_ANIM_PATH = "res://Assets/gun-idle1.png";
	const char* FIRE_ANIM_PATH = "res://Assets/fire1.png";
	const char* KICKBACK_ANIM_PATH = "res://Assets/fire2.png";
	const char* RECOVER_ANIM_PATH = "res://Assets/fire3.png";
	
	
};


#endif //_F_P_S_CONTROLLER_H
