#include "Lua_UI_Manager.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Scripting::Lua
{
namespace
{
Lua_UI_Action_Result ResultFromCallback(sol::protected_function_result result)
{
    if (result.valid())
        return {true, {}};

    const sol::error error = result;
    return {false, error.what()};
}
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddSection(
    std::uint64_t ownerScriptId,
    std::string label)
{
    return AddPassive(ownerScriptId, Lua_UI_Element_Type::Section, std::move(label));
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddText(
    std::uint64_t ownerScriptId,
    std::string text)
{
    return AddPassive(ownerScriptId, Lua_UI_Element_Type::Text, std::move(text));
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddButton(
    std::uint64_t ownerScriptId,
    std::string label,
    sol::protected_function callback)
{
    if (label.empty() || !callback.valid())
        return 0;

    const auto id = m_nextId++;
    m_elements.push_back({
        id,
        ownerScriptId,
        Lua_UI_Element_Type::Button,
        std::move(label),
        false,
        0.0F,
        0.0F,
        0.0F,
        std::move(callback)});
    return id;
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddCheckbox(
    std::uint64_t ownerScriptId,
    std::string label,
    bool value,
    sol::protected_function callback)
{
    if (label.empty() || !callback.valid())
        return 0;

    const auto id = m_nextId++;
    m_elements.push_back({
        id,
        ownerScriptId,
        Lua_UI_Element_Type::Checkbox,
        std::move(label),
        value,
        0.0F,
        0.0F,
        0.0F,
        std::move(callback)});
    return id;
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddSliderFloat(
    std::uint64_t ownerScriptId,
    std::string label,
    float value,
    float minValue,
    float maxValue,
    sol::protected_function callback)
{
    if (label.empty() || !callback.valid() || minValue > maxValue)
        return 0;

    const auto id = m_nextId++;
    m_elements.push_back({
        id,
        ownerScriptId,
        Lua_UI_Element_Type::SliderFloat,
        std::move(label),
        false,
        (std::clamp)(value, minValue, maxValue),
        minValue,
        maxValue,
        std::move(callback)});
    return id;
}

bool Lua_UI_Manager::Remove(std::uint64_t ownerScriptId, Element_Id id) noexcept
{
    const auto it = std::find_if(
        m_elements.begin(),
        m_elements.end(),
        [ownerScriptId, id](const Element& element) {
            return element.id == id && element.ownerScriptId == ownerScriptId;
        });
    if (it == m_elements.end())
        return false;

    m_elements.erase(it);
    return true;
}

std::size_t Lua_UI_Manager::RemoveByOwner(std::uint64_t ownerScriptId) noexcept
{
    const auto oldSize = m_elements.size();
    std::erase_if(m_elements, [ownerScriptId](const Element& element) {
        return element.ownerScriptId == ownerScriptId;
    });
    return oldSize - m_elements.size();
}

void Lua_UI_Manager::Clear() noexcept
{
    m_elements.clear();
}

std::size_t Lua_UI_Manager::Count() const noexcept
{
    return m_elements.size();
}

std::size_t Lua_UI_Manager::CountByOwner(std::uint64_t ownerScriptId) const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_elements.begin(),
        m_elements.end(),
        [ownerScriptId](const Element& element) {
            return element.ownerScriptId == ownerScriptId;
        }));
}

std::vector<Lua_UI_Element_Snapshot> Lua_UI_Manager::Snapshot() const
{
    std::vector<Lua_UI_Element_Snapshot> snapshot;
    snapshot.reserve(m_elements.size());

    for (const auto& element : m_elements) {
        snapshot.push_back({
            element.id,
            element.ownerScriptId,
            element.type,
            element.label,
            element.boolValue,
            element.floatValue,
            element.minValue,
            element.maxValue});
    }

    return snapshot;
}

Lua_UI_Action_Result Lua_UI_Manager::Activate(Element_Id id)
{
    auto* element = Find(id);
    if (!element || element->type != Lua_UI_Element_Type::Button)
        return {false, "Lua UI button was not found"};

    auto callback = element->callback;
    return ResultFromCallback(callback());
}

Lua_UI_Action_Result Lua_UI_Manager::SetCheckbox(Element_Id id, bool value)
{
    auto* element = Find(id);
    if (!element || element->type != Lua_UI_Element_Type::Checkbox)
        return {false, "Lua UI checkbox was not found"};

    element->boolValue = value;
    auto callback = element->callback;
    return ResultFromCallback(callback(value));
}

Lua_UI_Action_Result Lua_UI_Manager::SetSliderFloat(Element_Id id, float value)
{
    auto* element = Find(id);
    if (!element || element->type != Lua_UI_Element_Type::SliderFloat)
        return {false, "Lua UI slider was not found"};

    element->floatValue = (std::clamp)(value, element->minValue, element->maxValue);
    const float current = element->floatValue;
    auto callback = element->callback;
    return ResultFromCallback(callback(current));
}

Lua_UI_Manager::Element* Lua_UI_Manager::Find(Element_Id id) noexcept
{
    const auto it = std::find_if(
        m_elements.begin(),
        m_elements.end(),
        [id](const Element& element) { return element.id == id; });
    return it == m_elements.end() ? nullptr : &*it;
}

const Lua_UI_Manager::Element* Lua_UI_Manager::Find(Element_Id id) const noexcept
{
    const auto it = std::find_if(
        m_elements.begin(),
        m_elements.end(),
        [id](const Element& element) { return element.id == id; });
    return it == m_elements.end() ? nullptr : &*it;
}

Lua_UI_Manager::Element_Id Lua_UI_Manager::AddPassive(
    std::uint64_t ownerScriptId,
    Lua_UI_Element_Type type,
    std::string label)
{
    if (label.empty())
        return 0;

    const auto id = m_nextId++;
    m_elements.push_back({
        id,
        ownerScriptId,
        type,
        std::move(label),
        false,
        0.0F,
        0.0F,
        0.0F,
        {}});
    return id;
}
}
