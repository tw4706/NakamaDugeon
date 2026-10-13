#pragma once

class SceneManager;
class Scene
{
public:
	Scene(SceneManager& manager);
	virtual ~Scene();

	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

protected:
	SceneManager& sceneManager_;
};
