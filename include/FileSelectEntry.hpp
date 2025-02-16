//FileSelectEntry.hpp
#ifndef FILE_SELECT_ENTRY_HPP
#define FILE_SELECT_ENTRY_HPP

#include <tesla.hpp>
#include <string>
#include <filesystem>
#include <FileSelect.hpp>
#include <TextReader.hpp>
#include <Utils.hpp>
#include "imageviewer.hpp"

namespace fs = std::filesystem;

enum class FileType {
    TEXT,
    IMAGE, // ✅ Ensure Images support is included
    PDF,  // ✅ Ensure PDF support is included
    UNKNOWN
};

class FileSelectEntry {
public:
    FileSelectEntry(std::string const& path, bool favorite)
        : m_path(path), m_favorite(favorite) {}

    virtual ~FileSelectEntry() {}

    std::string getPath() const { return getPathInternal(); }
    std::string label() const {
        return (isFavorite() ? "\u2605 " : "") + getName();
    }

    void toggleFavorite();
    bool isFavorite() const { return m_favorite; }
    bool operator<(FileSelectEntry const& other) const;
    virtual std::string getName() const = 0;
    virtual void select() = 0;

    // Function to identify file type
    FileType getFileType() const;

protected:
    fs::path getPathInternal() const { return m_path; }
    virtual int ordering() const = 0;

private:
    fs::path m_path;
    bool m_favorite;
};

class FileSelectDirEntry : public FileSelectEntry {
public:
    FileSelectDirEntry(std::string const& path, bool favorite)
        : FileSelectEntry(path, favorite) {}

    std::string getName() const {
        return getPathInternal().parent_path().filename().string() + "/";
    }

    void select() override {
        tsl::changeTo<FileSelect>(getPath());
    }

protected:
    int ordering() const override;
};

class FileSelectFileEntry : public FileSelectEntry {
public:
    FileSelectFileEntry(std::string const& path, bool favorite)
        : FileSelectEntry(path, favorite) {}

    std::string getName() const {
        return getPathInternal().filename().string();
    }

    void select() override;

protected:
    int ordering() const override;
};

#endif // FILE_SELECT_ENTRY_HPP