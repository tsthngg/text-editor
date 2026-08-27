#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>

typedef struct {
    char *data;
    int length;
    int cap;
} Buffer;

typedef struct {
    int pos;
    int prefer_col;
    int anchor;
} Cursor;

// typedef struct {
//     char *data;
//     int length;
//     int cursor_pos;
// } EditorState;

// typedef struct {
//     EditorState *states;
//     int count;
//     int capacity;
//     int current;
// } History;

typedef struct {
    Buffer buffer;
    Cursor cursor;
    // History history;
    char filename[256];
} Editor;


// ---- Buffer -----
int buffer_init(Buffer *buffer);
int buffer_insert(Buffer *buffer, int pos, char c);
void clear_buffer(Buffer *buffer);

// ---- Display ----
void insert_char(Editor *editor, char c);
void editor_render(HDC hdc, Editor *editor);

// ---- Cursor ----
void get_cursor_pos(Editor *editor, int *line, int *col);
void get_cursor_screen_pos(HDC hdc, Editor *editor, int *x, int *y);
void change_prefer_col(Editor *editor);
int has_selection(Editor *editor);
void delete_selection(Editor *editor);

// ---- File ----
void save_file(HWND hwnd, Editor *editor);
void load_file(HWND hwnd, Editor *editor);
void new_file(Editor *editor);
int open_file_dialog(HWND hwnd, char *filename);
int save_file_dialog(HWND hwnd, char *filename);

// ---- Functional buttons ----
void cursor_left(Editor *editor, int selecting);
void cursor_right(Editor *editor, int selecting);
void cursor_up(Editor *editor);
void cursor_down(Editor *editor);
void backspace(Editor *editor);
void delete_button(Editor *editor);
void tab_button(Editor *editor);
void home(Editor *editor);
void end(Editor *editor);
void copy_selection(Editor *editor);
void paste_clipboard(Editor *editor);
// void history_init(History *history);
// void save_state(Editor *editor);
// void undo(Editor *editor);
// void redo(Editor *editor);

// ---- Windows procedure ----
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        Editor *editor = (Editor *)cs->lpCreateParams;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)editor);

        return TRUE;
    }
    Editor *editor =
        (Editor *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

            editor_render(hdc, editor);
            EndPaint(hwnd, &ps);

            return 0;
        }
        case WM_CHAR:
        {
            char c = (char)wParam;
            if (c == '\r') c = '\n';
            else if (c == '\b') return 0;
            else if (c == '\t') return 0;
            else if (c < 32) return 0;
            insert_char(editor, c);

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;   
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_BACK) backspace(editor);
            else if (wParam == VK_LEFT) cursor_left(editor, GetKeyState(VK_SHIFT) & 0x8000);
            else if (wParam == VK_RIGHT) cursor_right(editor, GetKeyState(VK_SHIFT) & 0x8000);
            else if (wParam == VK_UP) cursor_up(editor);
            else if (wParam == VK_DOWN) cursor_down(editor);
            else if (wParam == VK_DELETE) delete_button(editor);
            else if (wParam == VK_TAB) tab_button(editor);
            else if (wParam == VK_HOME) home(editor);
            else if (wParam == VK_END) end(editor);
            else if (wParam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) save_file(hwnd, editor);
            else if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) load_file(hwnd, editor);
            else if (wParam == 'N' && (GetKeyState(VK_CONTROL) & 0x8000)) new_file(editor);
            else if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                editor->cursor.anchor = 0;
                editor->cursor.pos = editor->buffer.length;
                change_prefer_col(editor);
            }
            else if (wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) copy_selection(editor);
            else if (wParam == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) paste_clipboard(editor);
            else if (wParam == 'X' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                copy_selection(editor);
                delete_selection(editor);
            }
            else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) redo(editor);
            else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) undo(editor);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ---- Main ----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Editor editor;
    if (!buffer_init(&editor.buffer)) return 1;
    editor.cursor.pos = 0;
    editor.cursor.prefer_col = 0;
    editor.cursor.anchor = 0;
    editor.filename[0] = '\0';
    // history_init(&editor.history);

    WNDCLASS wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Tedt";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "Tedt",
        "Text Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        hInstance,
        &editor
    );
    if (hwnd == NULL) {
        MessageBox(NULL, "CreateWindow failed!", "Error", MB_OK);
        return 1;
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    free(editor.buffer.data);
    return 0;
}

int buffer_init(Buffer *buffer) {
    buffer->data = malloc(16);
    if (buffer->data == NULL) {
        printf("Memory allocation failed!");
        return 0;
    }
    buffer->length = 0;
    buffer->cap = 16;
    buffer->data[0] = '\0';
    return 1;
}

int buffer_insert(Buffer *buffer, int pos, char c) {
    char* temp;
    if (buffer->length + 1 >= buffer->cap) {
        temp = realloc(buffer->data, buffer->cap*2);
        if (temp == NULL) {
            printf("Memory allocation failed!");
            return 0;
        } 
        buffer->data = temp;
        buffer->cap *= 2;
    }
    memmove(&buffer->data[pos + 1], &buffer->data[pos], buffer->length - pos + 1);
    buffer->data[pos] = c;
    buffer->length++;
    return 1;
}

void clear_buffer(Buffer *buffer) {
    buffer->length = 0;
    buffer->data[0] = '\0';
}

void get_cursor_pos(Editor *editor, int *line, int *col) {
    *line = *col = 0;
    for (int i = 0; i < editor->cursor.pos; i++) {
        if (editor->buffer.data[i] == '\n') {
            (*line)++;
            (*col) = 0;
        }
        else (*col)++;
    }
}

void get_cursor_screen_pos(HDC hdc, Editor *editor, int *x, int *y) {
    *x = 10;
    *y = 10;
    for (int i = 0; i < editor->cursor.pos; i++) {
        if (editor->buffer.data[i] == '\n') {
            *x = 10;
            *y += 20;
            continue;
        }

        SIZE size;
        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);
        *x += size.cx;
    }
}

void change_prefer_col(Editor *editor) {
    int line, col;
    get_cursor_pos(editor, &line, &col);
    editor->cursor.prefer_col = col;
}

int has_selection(Editor *editor) {
    return editor->cursor.pos != editor->cursor.anchor;
}

void delete_selection(Editor *editor) {
    int start = editor->cursor.anchor < editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    int end = editor->cursor.anchor > editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    memmove(&editor->buffer.data[start], &editor->buffer.data[end], editor->buffer.length - end + 1);
    editor->buffer.length -= (end - start);
    editor->cursor.pos = start;
    editor->cursor.anchor = start;
    change_prefer_col(editor);
}

void backspace(Editor *editor) {
    if (has_selection(editor)) {
        delete_selection(editor);
        return;
    }
    if (editor->cursor.pos == 0 || editor->buffer.length == 0) return;
    memmove(&editor->buffer.data[editor->cursor.pos-1], &editor->buffer.data[editor->cursor.pos], editor->buffer.length - editor->cursor.pos + 1);
    editor->buffer.length--;
    editor->cursor.pos--;
    editor->cursor.anchor = editor->cursor.pos;
    change_prefer_col(editor);
}

void cursor_left(Editor *editor, int selecting) {
    if (editor->cursor.pos > 0) editor->cursor.pos--;

    if (!selecting) {
        editor->cursor.anchor = editor->cursor.pos;
        change_prefer_col(editor);
    }
}

void cursor_right(Editor *editor, int selecting) {
    if (editor->cursor.pos < editor->buffer.length) editor->cursor.pos++;

    if (!selecting) {
        editor->cursor.anchor = editor->cursor.pos;
        change_prefer_col(editor);
    }
}

void cursor_up(Editor *editor) {
    int cur_start = editor->cursor.pos;
    while (cur_start > 0 && editor->buffer.data[cur_start - 1] != '\n') cur_start--;
    if (cur_start == 0) return;
    int prev_start = cur_start - 1;
    while (prev_start > 0 && editor->buffer.data[prev_start - 1] != '\n') prev_start--;
    
    int prev_length = 0;
    while (prev_start + prev_length < editor->buffer.length &&
       editor->buffer.data[prev_start + prev_length] != '\n') prev_length++;
    
    int new_col = editor->cursor.prefer_col;
    if (new_col > prev_length) new_col = prev_length;
    editor->cursor.pos = prev_start + new_col;
}

void cursor_down(Editor *editor) {
    int cur_end = editor->cursor.pos;
    while (cur_end < editor->buffer.length && editor->buffer.data[cur_end] != '\n') cur_end++;
    if (cur_end == editor->buffer.length) return;
    int next_start = cur_end + 1;

    int next_length = 0;
    while (next_start + next_length < editor->buffer.length &&
    editor->buffer.data[next_start + next_length] != '\n') next_length++;
    
    int new_col = editor->cursor.prefer_col;
    if (new_col > next_length) new_col = next_length;
    editor->cursor.pos = next_start + new_col;
}

void delete_button(Editor *editor) {
    if (has_selection(editor)) {
        delete_selection(editor);
        return;
    }
    if (editor->cursor.pos >= editor->buffer.length) return;
    memmove(&editor->buffer.data[editor->cursor.pos], &editor->buffer.data[editor->cursor.pos + 1], editor->buffer.length - editor->cursor.pos);
    editor->buffer.length--;
}

void tab_button(Editor *editor) {
    for (int i = 0; i < 4; i++) {
        if (!buffer_insert(&editor->buffer, editor->cursor.pos, ' ')) {
            break;
        }
        editor->cursor.pos++;
        editor->cursor.anchor = editor->cursor.pos;
    }
    change_prefer_col(editor);
}

void home(Editor *editor) {
    int start = editor->cursor.pos;
    while (start > 0 && editor->buffer.data[start - 1] != '\n') start--;
    editor->cursor.pos = start;
    editor->cursor.anchor = start;
    change_prefer_col(editor);
}

void end(Editor *editor) {
    int end = editor->cursor.pos;
    while (end < editor->buffer.length && editor->buffer.data[end] != '\n') end++;
    editor->cursor.pos = end;
    editor->cursor.anchor = end;
    change_prefer_col(editor);
}

void insert_char(Editor *editor, char c) {
    if (buffer_insert(&editor->buffer, editor->cursor.pos, c)) {
        editor->cursor.pos++;
        editor->cursor.anchor = editor->cursor.pos;
        change_prefer_col(editor);
    }
}

void editor_render(HDC hdc, Editor *editor) {
    SetBkMode(hdc, TRANSPARENT);

    int x = 10;
    int y = 10;
    int start = editor->cursor.anchor < editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    int end = editor->cursor.anchor > editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    
    HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));

    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {
            x = 10;
            y += 20;
            continue;
        }

        SIZE size;
        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);

        if (i >= start && i < end) {
            RECT rect = {x, y, x + size.cx, y + 20};
            FillRect(hdc, &rect, hbr);
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else {
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }
        TextOutA(hdc, x, y, &editor->buffer.data[i], 1);
        x += size.cx;
    }
    DeleteObject(hbr);

    get_cursor_screen_pos(hdc, editor, &x, &y);
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + 20);
    
}

void save_file(HWND hwnd, Editor *editor) {
    if (!strlen(editor->filename)) {
        if (!save_file_dialog(hwnd, editor->filename)) return;
    }
    FILE *file = fopen(editor->filename, "w");

    if (file == NULL) {
        MessageBoxA(
            hwnd,
            "Cannot save file!",
            "Error",
            MB_OK | MB_ICONERROR
        );
        return;
    }

    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {
            fputc('\r', file);
            fputc('\n', file);
        }
        else fputc(editor->buffer.data[i], file);
    }

    fclose(file);
}

int save_file_dialog(HWND hwnd, char *filename) {
    OPENFILENAMEA ofn = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;

    ofn.lpstrFile = filename;
    ofn.nMaxFile = 256;

    ofn.lpstrFilter =
        "Text Files (*.txt)\0*.txt\0"
        "All Files (*.*)\0*.*\0";

    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = "txt";

    ofn.Flags =
        OFN_OVERWRITEPROMPT |
        OFN_PATHMUSTEXIST;

    return GetSaveFileNameA(&ofn);
}

void load_file(HWND hwnd, Editor *editor) {
    char filename[256] = {0};
    if (!open_file_dialog(hwnd, filename)) {
        return;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        MessageBoxA(
            hwnd,
            "Cannot open file!",
            "Error",
            MB_OK | MB_ICONERROR
        );
        return;
    }

    clear_buffer(&editor->buffer);
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\r') continue;
        buffer_insert(&editor->buffer, editor->buffer.length, (char) c);
    }
    fclose(file);
    strcpy(editor->filename, filename);
    editor->cursor.pos = editor->cursor.prefer_col = 0;
}

int open_file_dialog(HWND hwnd, char *filename) {
    OPENFILENAMEA ofn = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 256;
    ofn.lpstrFilter =
        "Text Files (*.txt)\0*.txt\0"
        "All Files (*.*)\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameA(&ofn);
}

void new_file(Editor *editor) {
    if (editor->buffer.length != 0) {
        int result = MessageBoxA(
            NULL,
            "Do you want to save changes?",
            "Confirm",
            MB_YESNOCANCEL | MB_ICONQUESTION
        );
        if (result == IDYES) {
            save_file(NULL, editor);
        }
        else if (result == IDCANCEL) {
            return;
        }
    }
    clear_buffer(&editor->buffer);
    editor->cursor.pos = editor->cursor.prefer_col = 0;
    editor->filename[0] = '\0';
}

void copy_selection(Editor *editor) {
    if (!has_selection(editor)) return;

    int start = editor->cursor.anchor < editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    int end = editor->cursor.anchor > editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;
    int length = end - start;

    char *buffer = malloc(length + 1);
    if (buffer == NULL) return;

    memcpy(buffer, &editor->buffer.data[start], length);
    buffer[length] = '\0';

    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
        if (hMem) {
            memcpy(GlobalLock(hMem), buffer, length + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }

    free(buffer);
}

void paste_clipboard(Editor *editor) {
    if (!OpenClipboard(NULL)) return;

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        CloseClipboard();
        return;
    }

    char *clipboardText = (char *)GlobalLock(hData);
    if (clipboardText == NULL) {
        CloseClipboard();
        return;
    }

    int length = strlen(clipboardText);
    for (int i = 0; i < length; i++) {
        insert_char(editor, clipboardText[i]);
    }

    GlobalUnlock(hData);
    CloseClipboard();
}

// void history_init(History *history) {
//     history->states = malloc(sizeof(EditorState) * 10);
//     for (int i = 0; i < 10; i++) {
//         history->states[i].data = NULL;
//         history->states[i].length = 0;
//         history->states[i].cursor_pos = 0;
//     }
//     if (history->states == NULL) {
//         printf("Memory allocation failed!");
//         exit(1);
//     }
//     history->count = 0;
//     history->capacity = 10;
//     history->current = -1;
// }

// void save_state(Editor *editor) {
//     if (editor->history.current < editor->history.count - 1) {
//         for (int i = editor->history.current + 1; i < editor->history.count; i++) {
//             free(editor->history.states[i].data);
//         }
//         editor->history.count = editor->history.current + 1;
//     }

//     if (editor->history.count >= editor->history.capacity) {
//         editor->history.capacity *= 2;
//         editor->history.states = realloc(editor->history.states, sizeof(EditorState) * editor->history.capacity);
//     }

//     EditorState *state = &editor->history.states[editor->history.count];
//     state->length = editor->buffer.length;
//     state->data = malloc(state->length + 1);
//     memcpy(state->data, editor->buffer.data, state->length + 1);
//     state->cursor_pos = editor->cursor.pos;

//     editor->history.current++;
//     editor->history.count++;
// }

// void undo(Editor *editor) {
//     if (editor->history.current <= 0) return;

//     editor->history.current--;
//     EditorState *state = &editor->history.states[editor->history.current];

//     clear_buffer(&editor->buffer);
//     buffer_insert(&editor->buffer, 0, '\0');
//     memcpy(editor->buffer.data, state->data, state->length + 1);
//     editor->buffer.length = state->length;
//     editor->cursor.pos = state->cursor_pos;
//     editor->cursor.anchor = state->cursor_pos;
//     change_prefer_col(editor);
// }

// void redo(Editor *editor) {
//     if (editor->history.current >= editor->history.count - 1) return;

//     editor->history.current++;
//     EditorState *state = &editor->history.states[editor->history.current];

//     clear_buffer(&editor->buffer);
//     buffer_insert(&editor->buffer, 0, '\0');
//     memcpy(editor->buffer.data, state->data, state->length + 1);
//     editor->buffer.length = state->length;
//     editor->cursor.pos = state->cursor_pos;
//     editor->cursor.anchor = state->cursor_pos;
//     change_prefer_col(editor);
// }