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
} Cursor;

typedef struct {
    Buffer buffer;
    Cursor cursor;
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

// ---- File ----
void save_file(HWND hwnd, Editor *editor);
void load_file(HWND hwnd, Editor *editor);
void new_file(Editor *editor);
int open_file_dialog(HWND hwnd, char *filename);
int save_file_dialog(HWND hwnd, char *filename);

// ---- Functional buttons ----
void cursor_left(Editor *editor);
void cursor_right(Editor *editor);
void cursor_up(Editor *editor);
void cursor_down(Editor *editor);
void backspace(Editor *editor);
void delete_button(Editor *editor);
void tab_button(Editor *editor);
void home(Editor *editor);
void end(Editor *editor);

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
            else if (c < 32 && c != '\n') return 0;
            else insert_char(editor, c);

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;   
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_BACK) backspace(editor);
            else if (wParam == VK_LEFT) cursor_left(editor);
            else if (wParam == VK_RIGHT) cursor_right(editor);
            else if (wParam == VK_UP) cursor_up(editor);
            else if (wParam == VK_DOWN) cursor_down(editor);
            else if (wParam == VK_DELETE) delete_button(editor);
            else if (wParam == VK_TAB) tab_button(editor);
            else if (wParam == VK_HOME) home(editor);
            else if (wParam == VK_END) end(editor);
            else if (wParam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) save_file(hwnd, editor);
            else if (wParam == 'O' && (GetKeyState(VK_CONTROL) & 0x8000)) load_file(hwnd, editor);
            else if (wParam == 'N' && (GetKeyState(VK_CONTROL) & 0x8000)) new_file(editor);

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
    editor.filename[0] = '\0';

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

void backspace(Editor *editor) {
    if (editor->cursor.pos == 0 || editor->buffer.length == 0) return;
    memmove(&editor->buffer.data[editor->cursor.pos-1], &editor->buffer.data[editor->cursor.pos], editor->buffer.length - editor->cursor.pos + 1);
    editor->buffer.length--;
    editor->cursor.pos--;
    change_prefer_col(editor);
}

void cursor_left(Editor *editor) {
    if (editor->cursor.pos > 0) {
        editor->cursor.pos--;
        change_prefer_col(editor);
    }
}

void cursor_right(Editor *editor) {
    if (editor->cursor.pos < editor->buffer.length) {
        editor->cursor.pos++;
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
    }
    change_prefer_col(editor);
}

void home(Editor *editor) {
    int start = editor->cursor.pos;
    while (start > 0 && editor->buffer.data[start - 1] != '\n') start--;
    editor->cursor.pos = start;
    change_prefer_col(editor);
}

void end(Editor *editor) {
    int end = editor->cursor.pos;
    while (end < editor->buffer.length && editor->buffer.data[end] != '\n') end++;
    editor->cursor.pos = end;
    change_prefer_col(editor);
}

void insert_char(Editor *editor, char c) {
    if (buffer_insert(&editor->buffer, editor->cursor.pos, c)) {
        editor->cursor.pos++;
        change_prefer_col(editor);
    }
}

void editor_render(HDC hdc, Editor *editor) {
    int x = 10;
    int y = 10;

    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {
            x = 10;
            y += 20;
            continue;
        }

        TextOutA(hdc, x, y, &editor->buffer.data[i], 1);
        SIZE size;
        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);

        x += size.cx;
    }
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