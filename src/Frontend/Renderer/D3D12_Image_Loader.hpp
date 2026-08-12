#pragma once

#include <imgui.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Devilz::Frontend::Renderer
{
class D3D12_Image_Loader final
{
public:
    static D3D12_Image_Loader& Instance() noexcept;

    [[nodiscard]] bool Load(const std::filesystem::path& path, std::string* error = nullptr) noexcept;
    void Clear() noexcept;
    void Shutdown() noexcept;

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] ImTextureRef Texture() noexcept;
    [[nodiscard]] int Width() const noexcept { return m_width; }
    [[nodiscard]] int Height() const noexcept { return m_height; }
    [[nodiscard]] const std::string& LoadedPath() const noexcept { return m_loadedPath; }

private:
    D3D12_Image_Loader() = default;
    void RetireActive() noexcept;
    void SweepRetired() noexcept;

    std::unique_ptr<ImTextureData> m_active;
    std::vector<std::unique_ptr<ImTextureData>> m_retired;
    std::string m_loadedPath;
    int m_width = 0;
    int m_height = 0;
};
}
