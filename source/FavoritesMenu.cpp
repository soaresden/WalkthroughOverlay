// FavoritesMenu.cpp
#include "FavoritesMenu.hpp"
#include "Config.hpp"
#include "Utils.hpp"

FavoritesMenuFrame::FavoritesMenuFrame(std::list<FileSelectEntry*>& entries)
    : tsl::elm::OverlayFrame("Favorites", ""), // Initialiser correctement la classe parent avec titre et sous-titre
    m_entries(entries) {
}

void FavoritesMenuFrame::draw(tsl::gfx::Renderer* renderer) {
    // Appeler d'abord la méthode parente pour dessiner le cadre de base
    tsl::elm::OverlayFrame::draw(renderer);

    // Ajouter nos éléments personnalisés
    renderer->fillScreen(renderer->a({ 0x0, 0x0, 0x0, 0xD }));
    renderer->drawString("Favorites", false, 20, 50, 30, renderer->a(0xFFFF));
    renderer->drawRect(15, 720 - 73, tsl::cfg::FramebufferWidth - 30, 1, renderer->a(0xFFFF));
    renderer->drawString("\uE0E3  Favorite", false, 30, 693, 23, renderer->a(0xFFFF));
}

FavoritesMenu::FavoritesMenu() {
    // Utiliser la méthode statique de Config
    auto favorites = Config::read()["favorites"];

    if (!favorites.is_array()) {
        favorites = {};
    }

    for (auto& val : favorites) {
        std::string path = val;
        if (utils::endsWith(path, "/"))
            m_entries.push_back(new FileSelectDirEntry(path, true));
        else
            m_entries.push_back(new FileSelectFileEntry(path, true));
    }

    m_entries.sort([](FileSelectEntry* a, FileSelectEntry* b) { return *a < *b; });
}

FavoritesMenu::~FavoritesMenu() {
    for (auto* entry : m_entries) {
        delete entry;
    }
}

tsl::elm::Element* FavoritesMenu::createUI() {
    auto frame = new FavoritesMenuFrame(m_entries);

    if (m_entries.empty()) {
        auto emptyListDrawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString("No favorites added yet", false, 20, 100, 16, renderer->a(0xFFFF));
            });

        frame->setContent(emptyListDrawer);
        return frame;
    }

    auto list = new tsl::elm::List();

    for (auto* entry : m_entries) {
        auto item = new tsl::elm::ListItem(entry->label());
        item->setClickListener([entry, item](s64 keys) {
            if (keys & HidNpadButton_A) {
                entry->select();
                return true;
            }
            if (keys & HidNpadButton_Y) {
                entry->toggleFavorite();
                item->setText(entry->label());
                return true;
            }
            return false;
            });

        list->addItem(item);
    }

    list->setBoundaries(40, 110, 0, 0);
    frame->setContent(list);

    return frame;
}