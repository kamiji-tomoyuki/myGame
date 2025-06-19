#include "TempObj.h"

void TempObj::Init()
{
	BaseObject::Init();
	BaseObject::CreateModel("tempObject.obj");
}

void TempObj::Update()
{
	BaseObject::Update();
}

void TempObj::Draw(const ViewProjection& viewProjection)
{
	BaseObject::Draw(viewProjection);
}
