// FavoritesMenu.hpp
#pragma once
#include <tesla.hpp>
#include <list>
#include "FileSelectEntry.hpp"
#include "Config.hpp"
#include "Utils.hpp"

// Ne pas s'auto-inclure !
// #include "FavoritesMenu.hpp"  <-- Supprimer cette ligne qui cause la récursion infinie

class FavoritesMenuFrame : public tsl::elm::OverlayFrame {
public:
    FavoritesMenuFrame(std::list<FileSelectEntry*>& entries);
    virtual void draw(tsl::gfx::Renderer* renderer) override;
private:
    std::list<FileSelectEntry*>& m_entries;
};

class FavoritesMenu : public tsl::Gui {
public:
    FavoritesMenu();
    ~FavoritesMenu();
    virtual tsl::elm::Element* createUI() override;
private:
    std::list<FileSelectEntry*> m_entries;
};