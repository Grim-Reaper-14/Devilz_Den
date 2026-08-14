#pragma once

namespace Devilz::Frontend::Supplemental
{
enum class Page
{
    None,
    Misc,
    Recovery,
    Debug
};

inline Page SelectedPage = Page::None;
}
