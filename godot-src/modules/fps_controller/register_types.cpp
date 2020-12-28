#include "register_types.h"

#include "core/object/class_db.h"
#include "FPSController.h"
#include "FPSEnemy.h"

void register_fps_controller_types()
{
	ClassDB::register_class<FPSController>();
	ClassDB::register_class<FPSEnemy>();
}

void unregister_fps_controller_types()
{
	
}