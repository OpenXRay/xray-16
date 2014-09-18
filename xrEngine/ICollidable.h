#pragma once

class	ENGINE_API	ICollisionForm;

class	ENGINE_API	ICollidable	{
public:
	struct 
	{
		ICollisionForm*			model;
	}							collidable;
public:
	ICollidable();
	virtual ~ICollidable();
};
