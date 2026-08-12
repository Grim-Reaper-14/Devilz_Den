#pragma once

#include <imgui.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Devilz::Frontend::Renderer
{
struct D3D12_Image_View
{
    ImTextureRef texture{};
    int width = 0;
    int height = 0;
    bool ready = false;
};

class D3D12_Image_Loader final
{
public:
    static D3D12_Image_Loader& Instance() noexcept;

    [[nodiscard]] bool EnsureLoaded(const std::filesystem::path& path, std::string* error = nullptr) noexcept;
    [[nodiscard]] bool Reload(const std::filesystem::path& path, std::string* error = nullptr) noexcept;
    [[nodiscard]] D3D12_Image_View View(const std::filesystem::path& path) noexcept;
    void Remove(const std::filesystem::path& path) noexcept;

    // Compatibility helpers for code paths that only need one currently-selected image.
    [[nodiscard]] bool Load(const std::filesystem::path& path, std::string* error = nullptr) noexcept;
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] ImTextureRef Texture() noexcept;
    [[nodiscard]] int Width() const noexcept { return m_activeWidth; }
    [[nodiscard]] int Height() const noexcept { return m_activeHeight; }
    [[nodiscard]] const std::string& LoadedPath() const noexcept { return m_loadedPath; }

    void Tick() noexcept;
    void Clear() noexcept;
    void Shutdown() noexcept;

private:
    struct Image_Record
    {
        std::filesystem::path path;
        std::unique_ptr<ImTextureData> texture;
        std::filesystem::file_time_type lastWrite{};
        int width = 0;
        int height = 0;
    };

    D3D12_Image_Loader() = default;

    [[nodiscard]] static std::string KeyFor(const std::filesystem::path& path);
    [[nodiscard]] bool DecodeAndRegister(
        const std::filesystem::path& path,
        Image_Record& record,
        std::string* error) noexcept;
    void Retire(std::unique_ptr<ImTextureData> texture) noexcept;
    void SweepRetired() noexcept;

    std::unordered_map<std::string, Image_Record> m_images;
    std::vector<std::unique_ptr<ImTextureData>> m_retired;
    std::string m_activeKey;
    std::string m_loadedPath;
    int m_activeWidth = 0;
    int m_activeHeight = 0;
};
}
