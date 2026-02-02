#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "SDL3/SDL_rect.h"
#include "animation.h"

enum class PlayerState
{
	idle, running, jumping
};

struct PlayerData
{
	PlayerState state;
	PlayerData()
	{
		state = PlayerState::idle;
	}
};

struct LevelData {};
struct EnemyData {};

union ObjectData
{
	PlayerData player;
	LevelData level;
	EnemyData enemy;
};

enum class ObjectType
{
	player, level, enemy
};

struct GameObject
{
	ObjectType type;
	ObjectData data;
	glm::vec2 position, velocity, acceleration;
	float direction;
	float maxSpeedX;
	std::vector<Animation> animations;
	int currentAnimation;
	SDL_Texture *texture;
	bool dynamic;
	bool grounded;
	SDL_FRect collider;

	GameObject() : data { .level = LevelData() }, collider { 0 }
	{
		type = ObjectType::level;
		direction = 1;
		maxSpeedX = 0;
		position = velocity = acceleration = glm::vec2(0);
		currentAnimation = -1;
		texture = nullptr;
		dynamic = false;
		grounded = false;
	}
};
