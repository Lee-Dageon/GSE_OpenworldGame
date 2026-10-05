#include "stdafx.h"
#include "Actor.h"

#include <utility>

Actor::Actor(std::string name) : m_Name(std::move(name)) {}

Actor::~Actor() = default;

const std::string &Actor::GetName() const
{
    return m_Name;
}

Actor *Actor::GetParent() const
{
    return m_Parent;
}

void Actor::SetLocalPosition(ActorPosition position)
{
    m_LocalPosition = position;
}

ActorPosition Actor::GetLocalPosition() const
{
    return m_LocalPosition;
}

ActorPosition Actor::GetWorldPosition() const
{
    if (m_Parent == nullptr)
        return m_LocalPosition;

    const ActorPosition parentPosition = m_Parent->GetWorldPosition();
    return {parentPosition.x + m_LocalPosition.x, parentPosition.z + m_LocalPosition.z};
}

Actor *Actor::AddChild(std::unique_ptr<Actor> child)
{
    if (child == nullptr)
        return nullptr;

    child->m_Parent = this;
    Actor *childPointer = child.get();
    m_Children.push_back(std::move(child));
    return childPointer;
}

void Actor::Update(float deltaTime)
{
    OnUpdate(deltaTime);
    for (const std::unique_ptr<Actor> &child : m_Children)
        child->Update(deltaTime);
}

void Actor::Draw()
{
    OnDraw();
    for (const std::unique_ptr<Actor> &child : m_Children)
        child->Draw();
}

void Actor::OnUpdate(float) {}

void Actor::OnDraw() {}
