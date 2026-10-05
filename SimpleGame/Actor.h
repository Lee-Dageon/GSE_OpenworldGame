#pragma once

#include <memory>
#include <string>
#include <vector>

struct ActorPosition
{
    float x = 0.0f;
    float z = 0.0f;
};

// 화면에 배치되는 모든 게임 오브젝트의 공통 Scene Graph 노드입니다.
class Actor
{
  public:
    explicit Actor(std::string name = "Actor");
    virtual ~Actor();

    Actor(const Actor &) = delete;
    Actor &operator=(const Actor &) = delete;

    const std::string &GetName() const;
    Actor *GetParent() const;

    void SetLocalPosition(ActorPosition position);
    ActorPosition GetLocalPosition() const;
    ActorPosition GetWorldPosition() const;

    Actor *AddChild(std::unique_ptr<Actor> child);

    void Update(float deltaTime);
    void Draw();

  protected:
    virtual void OnUpdate(float deltaTime);
    virtual void OnDraw();

  private:
    std::string m_Name;
    ActorPosition m_LocalPosition;
    Actor *m_Parent = nullptr;
    std::vector<std::unique_ptr<Actor>> m_Children;
};
