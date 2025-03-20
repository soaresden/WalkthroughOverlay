//fileSelectEntry.hpp

#ifndef FILE_SELECT_ENTRY_HPP
#define FILE_SELECT_ENTRY_HPP
#pragma once
#include <tesla.hpp>
#include <string>
#include <filesystem>
#include <FileSelect.hpp>
#include <TextReader.hpp>
#include <Utils.hpp>
#include "ImageViewer.hpp"

enum class FileType {
    IMAGE,
    TEXT,
    UNKNOWN
};

class FileSelectEntry {
public:
    FileSelectEntry(std::string const& path, bool favorite)
        : m_path(std::filesystem::path(path)), m_favorite(favorite) {
    }
    virtual ~FileSelectEntry() {}

    std::string getPath() const {
        return m_path.string();
    }

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
    virtual int ordering() const = 0;
    std::filesystem::path m_path;

private:
    bool m_favorite;
};

class FileSelectDirEntry : public FileSelectEntry {
public:
    FileSelectDirEntry(std::string const& path, bool favorite)
        : FileSelectEntry(path, favorite) {
    }

    std::string getName() const override {
        return m_path.parent_path().filename().string() + "/";
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
        : FileSelectEntry(path, favorite) {
    }

    std::string getName() const override {
        return m_path.filename().string();
    }

    void select() override;

protected:
    int ordering() const override;
};

#endif // FILE_SELECT_ENTRY_HPP