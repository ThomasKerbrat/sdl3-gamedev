#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <array>
#include <string>
#include <vector>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_render.h"
#include "gameobject.h"

using namespace std;

struct SDLState
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	int width, height, logW, logH;
	bool const *keys;

	SDLState() : keys(SDL_GetKeyboardState(nullptr))
	{
	}
};

size_t const LAYER_IDX_LEVEL = 0;
size_t const LAYER_IDX_CHARACTERS = 1;
int const MAP_ROWS = 5;
int const MAP_COLS = 50;
int const TILE_SIZE = 32;

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	int playerIndex;

	GameState()
	{
		playerIndex = 0;
	}
};

struct Resources
{
	int const ANIM_PLAYER_IDLE = 0;
	int const ANIM_PLAYER_RUN = 1;
	std::vector<Animation> playerAnims;

	std::vector<SDL_Texture *> textures;
	SDL_Texture *texIdle, *texRun, *texBrick, *texGrass, *texGround, *texPanel;

	SDL_Texture *loadTextures(SDL_Renderer *renderer, std::string const &filepath)
	{
		SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState &state)
	{
		playerAnims.resize(5);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);

		texIdle = loadTextures(state.renderer, "data/idle.png");
		texRun = loadTextures(state.renderer, "data/run.png");
		texBrick = loadTextures(state.renderer, "data/tiles/brick.png");
		texGrass = loadTextures(state.renderer, "data/tiles/grass.png");
		texGround = loadTextures(state.renderer, "data/tiles/ground.png");
		texPanel = loadTextures(state.renderer, "data/tiles/panel.png");
	}

	void unload()
	{
		for (SDL_Texture *tex : textures)
		{
			SDL_DestroyTexture(tex);
		}
	}
};

bool initialize(SDLState &state);
void cleanup(SDLState &state);
void drawObject(SDLState const &state, GameState &gs, GameObject &obj, float deltaTime);
void update(SDLState const &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);
void createTiles(SDLState const &state, GameState &gs, Resources &res);

int main(int argc, char *argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;

	if (!initialize(state))
	{
		return 1;
	}

	// Load game assets
	Resources res;
	res.load(state);

	// Setup game data
	GameState gs;
	createTiles(state, gs, res);
	uint64_t prevTime = SDL_GetTicks();

	// Start the game loop
	bool running = true;
	while (running)
	{
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - prevTime) / 1000.0f;
		SDL_Event event { 0 };
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
				{
					running = false;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED:
				{
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
				}
			}
		}

		// Update all objects
		for (auto &layer : gs.layers)
		{
			for (GameObject &obj : layer)
			{
				update(state, gs, res, obj, deltaTime);

				// Update the animation
				if (obj.currentAnimation != -1)
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// Perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		// Draw all objects
		for (auto &layer : gs.layers)
		{
			for (GameObject &obj : layer)
			{
				drawObject(state, gs, obj, deltaTime);
			}
		}

		// Swap buffers and present
		SDL_RenderPresent(state.renderer);
		prevTime = nowTime;
	}

	res.unload();
	cleanup(state);
	return 0;
}

bool initialize(SDLState &state)
{
	bool initSuccess = true;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
		initSuccess = false;
	}

	// Create the window
	state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height, SDL_WINDOW_RESIZABLE);

	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", state.window);
		cleanup(state);
		initSuccess = false;
	}

	// Create the renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);

	if (!state.renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", state.window);
		cleanup(state);
		initSuccess = false;
	}

	// Configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return initSuccess;
}

void cleanup(SDLState &state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(SDLState const &state, GameState &gs, GameObject &obj, float deltaTime)
{
	float const spriteSize = 32;
	float srcX = obj.currentAnimation != -1
		? obj.animations[obj.currentAnimation].currentFrame() * spriteSize
		: 0.0f
	;

	SDL_FRect src {
		.x = srcX,
		.y = 0,
		.w = spriteSize,
		.h = spriteSize,
	};

	SDL_FRect dst {
		.x = obj.position.x,
		.y = obj.position.y,
		.w = spriteSize,
		.h = spriteSize,
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}

void update(SDLState const &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
	// Apply some gravity
	if (obj.dynamic)
	{
		obj.velocity += glm::vec2(0, 500) * deltaTime;
	}

	if (obj.type == ObjectType::player)
	{
		float currentDirection = 0;
		if (state.keys[SDL_SCANCODE_A])
		{
			currentDirection -= 1;
		}
		if (state.keys[SDL_SCANCODE_D])
		{
			currentDirection += 1;
		}
		if (currentDirection)
		{
			obj.direction = currentDirection;
		}

		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (currentDirection)
				{
					obj.data.player.state = PlayerState::running;
					obj.texture = res.texRun;
					obj.currentAnimation = res.ANIM_PLAYER_RUN;
				}
				else
				{
					// Decelerate
					if (obj.velocity.x)
					{
						float const factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
						float amount = factor * obj.acceleration.x * deltaTime;
						if (std::abs(obj.velocity.x) < std::abs(amount))
						{
							obj.velocity.x = 0;
						}
						else
						{
							obj.velocity.x += amount;
						}
					}
				}
				break;
			}
			case PlayerState::running:
			{
				if (!currentDirection)
				{
					obj.data.player.state = PlayerState::idle;
					obj.texture = res.texIdle;
					obj.currentAnimation = res.ANIM_PLAYER_IDLE;
				}
				break;
			}
		}

		// Add acceleration to velocity
		obj.velocity += currentDirection * obj.acceleration * deltaTime;
		if (std::abs(obj.velocity.x) > obj.maxSpeedX)
		{
			obj.velocity.x = currentDirection * obj.maxSpeedX;
		}

		// Add velocity to position
		obj.position += obj.velocity * deltaTime;
	}
}

void createTiles(SDLState const &state, GameState &gs, Resources &res)
{
	/*
		1 - Ground
		2 - Panel
		3 - Enemy
		4 - Player
		5 - Grass
		6 - Brick
	*/
	short map[MAP_ROWS][MAP_COLS] = {
		4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	auto const createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
	{
		GameObject o;
		o.type = type;
		o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
		o.texture = tex;
		return o;
	};

	for (int r = 0; r < MAP_ROWS; r++)
	{
		for (int c = 0; c < MAP_COLS; c++)
		{
			switch (map[r][c])
			{
				case 1: // ground
				{
					GameObject ground = createObject(r, c, res.texGround, ObjectType::level);
					gs.layers[LAYER_IDX_LEVEL].push_back(ground);
					break;
				}
				case 2: // ground
				{
					GameObject panel = createObject(r, c, res.texPanel, ObjectType::level);
					gs.layers[LAYER_IDX_LEVEL].push_back(panel);
					break;
				}
				case 4: // player
				{
					GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
					player.data.player = PlayerData();
					player.animations = res.playerAnims;
					player.currentAnimation = res.ANIM_PLAYER_IDLE;
					player.acceleration = glm::vec2(300, 0);
					player.maxSpeedX = 100;
					player.dynamic = true;
					gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
					break;
				}
			}
		}
	}
}
