// Files.cpp
#include "Files.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace files {

    std::string lowerExt(const std::string& path) {
        auto dot = path.find_last_of('.');
        auto slash = path.find_last_of('/');
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
        std::string ext = path.substr(dot);
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        return ext;
    }

    bool isText(const std::string& path) {
        std::string e = lowerExt(path);
        return e == ".txt" || e == ".md" || e == ".log" || e == ".ini" || e == ".nfo";
    }

    bool isImage(const std::string& path) {
        std::string e = lowerExt(path);
        return e == ".png" || e == ".jpg" || e == ".jpeg";
    }

    Kind kindOf(const std::string& path) {
        if (!path.empty() && path.back() == '/') return Kind::Dir;
        if (isText(path)) return Kind::Text;
        if (isImage(path)) return Kind::Image;
        return Kind::Other;
    }

    bool naturalLess(const std::string& a, const std::string& b) {
        size_t i = 0, j = 0;
        while (i < a.size() && j < b.size()) {
            unsigned char ca = a[i], cb = b[j];
            if (std::isdigit(ca) && std::isdigit(cb)) {
                // compare the whole digit runs numerically
                size_t si = i, sj = j;
                while (si < a.size() && a[si] == '0') ++si;
                while (sj < b.size() && b[sj] == '0') ++sj;
                size_t ei = si, ej = sj;
                while (ei < a.size() && std::isdigit((unsigned char)a[ei])) ++ei;
                while (ej < b.size() && std::isdigit((unsigned char)b[ej])) ++ej;
                size_t la = ei - si, lb = ej - sj;
                if (la != lb) return la < lb;
                int c = a.compare(si, la, b, sj, lb);
                if (c != 0) return c < 0;
                i = ei; j = ej;
                continue;
            }
            unsigned char la = std::tolower(ca), lb = std::tolower(cb);
            if (la != lb) return la < lb;
            ++i; ++j;
        }
        return (a.size() - i) < (b.size() - j);
    }

    bool exists(const std::string& path) {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    bool isDirectory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    void mkdirs(const std::string& dirPath) {
        std::string cur;
        for (size_t i = 0; i < dirPath.size(); ++i) {
            cur += dirPath[i];
            if (dirPath[i] == '/' && i > 6) // skip "sdmc:/"
                mkdir(cur.substr(0, cur.size() - 1).c_str(), 0777);
        }
    }

    std::string fileName(const std::string& path) {
        std::string p = path;
        if (!p.empty() && p.back() == '/') p.pop_back();
        auto slash = p.find_last_of('/');
        return slash == std::string::npos ? p : p.substr(slash + 1);
    }

    std::string parentDir(const std::string& path) {
        std::string p = path;
        if (!p.empty() && p.back() == '/') p.pop_back();
        auto slash = p.find_last_of('/');
        if (slash == std::string::npos) return "";
        return p.substr(0, slash + 1);
    }

    std::vector<Entry> list(const std::string& dir, bool includeOther) {
        std::vector<Entry> dirs, filesOut;
        DIR* d = opendir(dir.c_str());
        if (!d) return {};
        struct dirent* ent;
        while ((ent = readdir(d)) != nullptr) {
            std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            if (!name.empty() && name[0] == '.') continue; // hidden / .overlays etc.
            std::string full = dir + name;
            bool isDir = (ent->d_type == DT_DIR);
            if (ent->d_type == DT_UNKNOWN) isDir = isDirectory(full);
            if (isDir) {
                dirs.push_back({ full + "/", name + "/", Kind::Dir });
            } else {
                Kind k = kindOf(full);
                if (k == Kind::Other && !includeOther) continue;
                filesOut.push_back({ full, name, k });
            }
        }
        closedir(d);

        auto byName = [](const Entry& a, const Entry& b) { return naturalLess(a.name, b.name); };
        std::sort(dirs.begin(), dirs.end(), byName);
        std::sort(filesOut.begin(), filesOut.end(), byName);
        dirs.insert(dirs.end(), filesOut.begin(), filesOut.end());
        return dirs;
    }

    std::vector<std::string> imagesIn(const std::string& dir) {
        std::vector<std::string> out;
        for (auto& e : list(dir))
            if (e.kind == Kind::Image) out.push_back(e.path);
        return out;
    }
}
