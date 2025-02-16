# 🕹️ Walkthrough Overlay  

![GitHub Repo Size](https://img.shields.io/github/repo-size/soaresden/WalkthroughOverlay?style=for-the-badge)  
![GitHub Last Commit](https://img.shields.io/github/last-commit/soaresden/WalkthroughOverlay?style=for-the-badge)  
![GitHub Issues](https://img.shields.io/github/issues/soaresden/WalkthroughOverlay?style=for-the-badge)  
![GitHub License](https://img.shields.io/github/license/soaresden/WalkthroughOverlay?style=for-the-badge)  

## 📌 **Description**  
**Walkthrough Overlay** is a **homebrew overlay** for the Nintendo Switch that allows displaying **TXT, PDF, and images** while in-game.  
Developed using **libtesla**, this overlay provides an easy way to view text guides, images, and documents without leaving your game.  

---

## 🛠 **Features**  

✅ **Read TXT files** with bookmarking and adjustable text size  
✅ **View images (.jpg, .jpeg, .png)** with zoom and navigation  
✅ **Display PDF files** with zoom and scrolling  
✅ **Smooth navigation** using Joy-Con and buttons  
✅ **Auto-save last read position** in files  
✅ **Minimalist and intuitive UI**  

---

## 🎮 **Controls**  

| 🕹️ **Action** | 🎮 **Button** |
|--------------|--------------|
| **Vertical Scrolling** | 🎛️ Right Stick Up / Down |
| **Move in text** | 🎛️ Left Stick |
| **Zoom In / Out** | 🔄 Left / Right Button |
| **Reset text alignment** | 🔄 Right Stick Press |
| **Add Bookmark** | ⭐ Y Button |
| **Go to Previous / Next Bookmark** | ⏩ L / R |
| **Exit Overlay** | ❌ B Button |

---

## 📦 **Installation**  

### **Requirements**  
- **Nintendo Switch with CFW** and **Tesla Menu** installed  
- **MicroSD Card** with **Atmosphère**  

### **Installation Steps**  

1️⃣ Download the latest release from the **[Releases](https://github.com/soaresden/WalkthroughOverlay/releases)** section  
2️⃣ Copy **WalkthroughOverlay.ovl** to **sd:/switch/.overlays/**  
3️⃣ Launch a game and **open Tesla Menu** (`L + D-Pad Down + Right Stick`)  
4️⃣ Select **Walkthrough Overlay** and start reading!  

---

## 🖥️ **Building from Source**  

### **Dependencies**  
- **[DevkitPro](https://devkitpro.org/)**
- **[libtesla](https://github.com/XorTroll/tesla)**
- **[libnx](https://github.com/switchbrew/libnx)**
- **[stb_image](https://github.com/nothings/stb)**
- **[MuPDF](https://mupdf.com/) ([GitHub](https://github.com/ArtifexSoftware/mupdf))**  

### **Build Instructions**  
use the AlreadyMade Batch if you have setup evrything already
!MakeFileBatchforwindows.bat

