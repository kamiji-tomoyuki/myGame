#include "MyGame.h"

using namespace Engine;
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	std::unique_ptr<Framework> game = std::make_unique<MyGame>();

	game->Run();

	return 0;
}
