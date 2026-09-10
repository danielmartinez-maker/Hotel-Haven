#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace hh::frontend {
class BuildCatalog {
public:
    BuildCatalog() = default; explicit BuildCatalog(std::vector<BuildCatalogItem> items) : items_(std::move(items)) {}
    void update(std::vector<BuildCatalogItem> items) { items_ = std::move(items); }
    [[nodiscard]] const std::vector<BuildCatalogItem>& items() const noexcept { return items_; }
    [[nodiscard]] std::vector<BuildCatalogItem> filter(const std::string& query, const std::string& category = {}) const;
private: std::vector<BuildCatalogItem> items_;
};
class BuildToolController {
public:
    void selectItem(std::string itemId) { selectedItemId_ = std::move(itemId); }
    [[nodiscard]] const std::string& selectedItem() const noexcept { return selectedItemId_; }
    void applyAuthoritativePreview(const BuildPlacementPreview& preview) { preview_ = preview; }
    [[nodiscard]] bool canConfirm() const noexcept { return preview_.valid && !preview_.itemId.empty(); }
    [[nodiscard]] const std::string& rejectionCode() const noexcept { return preview_.reasonCode; }
    [[nodiscard]] const std::string& rejectionText() const noexcept { return preview_.reasonText; }
    [[nodiscard]] const BuildPlacementPreview& preview() const noexcept { return preview_; }
    [[nodiscard]] std::optional<UiCommand> confirmCommand() const; [[nodiscard]] UiCommand rotateCommand() const; [[nodiscard]] UiCommand cancelCommand() const;
private: std::string selectedItemId_; BuildPlacementPreview preview_{};
};
} // namespace hh::frontend
