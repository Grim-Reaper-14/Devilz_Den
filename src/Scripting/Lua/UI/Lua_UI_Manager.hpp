#pragma once

#include "Lua_UI_Types.hpp"

#include <sol/sol.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Devilz::Scripting::Lua
{
class Lua_UI_Manager final
{
public:
    using Element_Id = Lua_UI_Element_Id;

    Element_Id AddSection(std::uint64_t ownerScriptId, std::string label);
    Element_Id AddText(std::uint64_t ownerScriptId, std::string text);
    Element_Id AddButton(
        std::uint64_t ownerScriptId,
        std::string label,
        sol::protected_function callback);
    Element_Id AddCheckbox(
        std::uint64_t ownerScriptId,
        std::string label,
        bool value,
        sol::protected_function callback);
    Element_Id AddSliderFloat(
        std::uint64_t ownerScriptId,
        std::string label,
        float value,
        float minValue,
        float maxValue,
        sol::protected_function callback);

    bool Remove(std::uint64_t ownerScriptId, Element_Id id) noexcept;
    std::size_t RemoveByOwner(std::uint64_t ownerScriptId) noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t CountByOwner(std::uint64_t ownerScriptId) const noexcept;
    [[nodiscard]] std::vector<Lua_UI_Element_Snapshot> Snapshot() const;

    [[nodiscard]] Lua_UI_Action_Result Activate(Element_Id id);
    [[nodiscard]] Lua_UI_Action_Result SetCheckbox(Element_Id id, bool value);
    [[nodiscard]] Lua_UI_Action_Result SetSliderFloat(Element_Id id, float value);

private:
    struct Element
    {
        Element_Id id{};
        std::uint64_t ownerScriptId{};
        Lua_UI_Element_Type type{Lua_UI_Element_Type::Text};
        std::string label;
        bool boolValue{};
        float floatValue{};
        float minValue{};
        float maxValue{};
        sol::protected_function callback;
    };

    [[nodiscard]] Element* Find(Element_Id id) noexcept;
    [[nodiscard]] const Element* Find(Element_Id id) const noexcept;
    [[nodiscard]] Element_Id AddPassive(
        std::uint64_t ownerScriptId,
        Lua_UI_Element_Type type,
        std::string label);

    Element_Id m_nextId{1};
    std::vector<Element> m_elements;
};
}
