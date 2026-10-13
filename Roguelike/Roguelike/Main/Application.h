#pragma once
class Application
{
	Application();
	Application(const Application& app) = delete;
	void operator=(const Application& app) = delete;

public:
	virtual ~Application();

	static Application& GetInstance();

	bool Init();
	void Run();
	void Terminate();

private:
	//揺れを付けるために一度描く画面
	int gameScreen_ = -1;
};
