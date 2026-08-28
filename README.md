# Text Editor
A lightweight, single-file plain text editor for Windows, written in C using the raw Win32 API.

## 📖 Overview
This is a minimal text editor just like Notepad, Microsoft Word but with less function. Built directly on top of the Windows API (windows.h) with no external GUI frameworks or libraries. It was created to get acquainted with C, Memory Allocation, Win32 API,...

## ✨ Features
- Basic text editing
- Cursor navigation & selection
- Work perfectly fine with files (Ctrl+S/Ctrl+O/Ctrl+N)
- Undo/Redo history

## 🚀 Getting Started
### Prerequisites
- Windows OS
- gcc (C Compiler)

### Installation

```bash
git clone https://github.com/tsthngg/text-editor
```

### Usage
- Build
```bash
gcc main.c -o editor.exe -luser32 -lgdi32 -lcomdlg32
```
- Run
```bash
./editor.exe
```

|Shortcut|Action|
| :---:|:---: |
|Ctrl+N|New file|
|Ctrl+O|Open file|
|Ctrl+S|Save file|
|Ctrl+A|Select all|
|Ctrl+C|Copy selection|
|Ctrl+X|Cut selection|
|Ctrl+V|Paste|
|Ctrl+Z|Undo|
|Ctrl+Shift+Z|Redo|
|Shift + Arrow keys|Extend selection|
|Tab|Insert 4 spaces|

### How It Works
- Buffer — a dynamically growing char* array that stores the full document as flat text
- Cursor — tracks the current position, a "preferred column" (for consistent vertical movement across lines of different lengths), and an anchor point used to compute the active selection.
- Rendering — on WM_PAINT, the buffer is walked character by character using GDI to draw text and highlight selected regions.
- File I/O — uses the standard Win32 dialogs; line endings are normalized

## 🤝 Contributing
Contributions are welcome! Feel free to open an issue or submit a pull request.

## 📄 License
Distributed under the MIT License. See [LICENSE](https://github.com/tsthngg/text-editor/blob/main/LICENSE) for more information.