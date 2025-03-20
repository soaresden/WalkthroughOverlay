// FileSelectEntry.cpp
#include "FileSelectEntry.hpp"
#include "Config.hpp"
#include <algorithm>

void FileSelectEntry::toggleFavorite() {
    m_favorite = !m_favorite;
    Config::update([this](nlohmann::json& j) {  // Utilisez le nom complet nlohmann::json
        if (!j["favorites"].is_array()) {
            j["favorites"] = {};
        }
        if (m_favorite) {
            j["favorites"].push_back(getPath());
        }
        else {
            auto found = std::find(j["favorites"].begin(), j["favorites"].end(), getPath());
            if (found != j["favorites"].end()) {
                j["favorites"].erase(found);
            }
        }
        });
}

bool FileSelectEntry::operator<(FileSelectEntry const& other) const {
    // Different types of entries
    if (ordering() != other.ordering()) {
        return ordering() < other.ordering();
    }

    // Both entries of the same type
    return getName() < other.getName();
}

int FileSelectDirEntry::ordering() const {
    return 0;
}

int FileSelectFileEntry::ordering() const {
    return 1;
}

FileType FileSelectEntry::getFileType() const {
    // Utiliser m_path au lieu de getPathInternal()
    std::string extension = m_path.extension().string();

    // Convertir l'extension en minuscules pour une comparaison insensible à la casse
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp" || extension == ".gif") {
        return FileType::IMAGE;
    }
    else if (extension == ".txt" || extension == ".md" || extension == ".ini") {
        return FileType::TEXT;
    }
    else {
        return FileType::UNKNOWN;
    }
}

void FileSelectFileEntry::select() {
    // Sélectionner l'action en fonction du type de fichier
    FileType type = getFileType();

    switch (type) {
    case FileType::IMAGE:
        tsl::changeTo<ImageViewer>(getPath());
        break;
    case FileType::TEXT:
        tsl::changeTo<TextReader>(getPath());
        break;
    default:
        // Type de fichier non pris en charge
        break;
    }
}