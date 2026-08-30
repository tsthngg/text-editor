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
    int selecting;
} Cursor;

typedef struct {
    char *data;
    int length;
    int cursor_pos;
} EditorState;

typedef struct {
    EditorState *states;
    int count;
    int capacity;
    int current;
} History;

typedef struct {
    Buffer buffer;
    Cursor cursor;
    History history;
    int scroll_y;
    char filename[256];
} Editor;


// ---- Buffer -----
int buffer_init(Buffer *buffer);
int buffer_insert(Buffer *buffer, int pos, char c);
void clear_buffer(Buffer *buffer);

// ---- Display ----
void insert_char(Editor *editor, char c);
void editor_render(HWND hwnd, HDC hdc, Editor *editor);
int get_word_end(Editor *editor, int start);

// ---- Cursor ----
void cursor_init(Cursor *cursor);
void get_cursor_pos(Editor *editor, int *line, int *col);
void get_cursor_screen_pos(HWND hwnd, HDC hdc, Editor *editor, int *x, int *y);
void change_prefer_col(Editor *editor);
int has_selection(Editor *editor);
void delete_selection(Editor *editor);
int get_cursor_fr_mouse(HWND hwnd, HDC hdc, Editor *editor, int mouse_x, int mouse_y);
int is_mouse_on_text(Editor *editor, int mouse_x, int mouse_y);
int get_line_count(Editor *editor);
void update_scrollbar(HWND hwnd, Editor *editor);
void ensure_cursor_visible(HWND hwnd, Editor *editor);

// ---- File ----
void save_file(HWND hwnd, Editor *editor);
void load_file(HWND hwnd, Editor *editor);
void new_file(Editor *editor);
int open_file_dialog(HWND hwnd, char *filename);
int save_file_dialog(HWND hwnd, char *filename);

// ---- Functional buttons ----
void cursor_left(Editor *editor, int selecting);
void cursor_right(Editor *editor, int selecting);
void cursor_up(Editor *editor, int selecting);
void cursor_down(Editor *editor, int selecting);
void backspace(Editor *editor);
void delete_button(Editor *editor);
void tab_button(Editor *editor);
void home(Editor *editor);
void end(Editor *editor);
void copy_selection(Editor *editor);
void paste_clipboard(Editor *editor);
void history_init(History *history);
void free_history(History *history);
void save_state(Editor *editor);
void undo(Editor *editor);
void redo(Editor *editor);

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
        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTVSCROLL) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            if (is_mouse_on_text(editor, pt.x, pt.y)) SetCursor(LoadCursor(NULL, IDC_IBEAM));
            else SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;
        }
        case WM_SIZE:
        {
            update_scrollbar(hwnd, editor);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

            editor_render(hwnd, hdc, editor);
            EndPaint(hwnd, &ps);

            return 0;
        }
        case WM_VSCROLL:
        {
            SCROLLINFO si = {0};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;

            GetScrollInfo(hwnd, SB_VERT, &si);

            int old_pos = si.nPos;

            switch (LOWORD(wParam)) {

                case SB_LINEUP:
                    si.nPos -= 1;
                    break;

                case SB_LINEDOWN:
                    si.nPos += 1;
                    break;

                case SB_PAGEUP:
                    si.nPos -= si.nPage;
                    break;

                case SB_PAGEDOWN:
                    si.nPos += si.nPage;
                    break;

                case SB_THUMBTRACK:
                    si.nPos = si.nTrackPos;
                    break;

                case SB_TOP:
                    si.nPos = si.nMin;
                    break;

                case SB_BOTTOM:
                    si.nPos = si.nMax - si.nPage;
                    break;
            }

            int max_scroll = si.nMax - si.nPage;

            if (max_scroll < 0)
                max_scroll = 0;

            if (si.nPos < si.nMin)
                si.nPos = si.nMin;

            if (si.nPos > max_scroll)
                si.nPos = max_scroll;

            si.fMask = SIF_POS;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

            GetScrollInfo(hwnd, SB_VERT, &si);

            if (si.nPos != old_pos) {
                editor->scroll_y = si.nPos;
                InvalidateRect(hwnd, NULL, TRUE);
            }

            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (delta > 0) editor->scroll_y -= 3;
            else if (delta < 0) editor->scroll_y += 3;

            SCROLLINFO si = {0};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;

            GetScrollInfo(hwnd, SB_VERT, &si);

            if (editor->scroll_y < si.nMin) editor->scroll_y = si.nMin;

            if (editor->scroll_y > si.nMax - (int)si.nPage + 1) editor->scroll_y = si.nMax - si.nPage + 1;

            if (editor->scroll_y < 0) editor->scroll_y = 0;

            si.fMask = SIF_POS;
            si.nPos = editor->scroll_y;

            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

            InvalidateRect(hwnd, NULL, TRUE);

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
            update_scrollbar(hwnd, editor);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;   
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_BACK) backspace(editor);
            else if (wParam == VK_LEFT) cursor_left(editor, GetKeyState(VK_SHIFT) & 0x8000);
            else if (wParam == VK_RIGHT) cursor_right(editor, GetKeyState(VK_SHIFT) & 0x8000);
            else if (wParam == VK_UP) cursor_up(editor, GetKeyState(VK_SHIFT) & 0x8000);
            else if (wParam == VK_DOWN) cursor_down(editor, GetKeyState(VK_SHIFT) & 0x8000);
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
                save_state(editor);
            }
            else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) redo(editor);
            else if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) undo(editor);
            
            ensure_cursor_visible(hwnd, editor);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            SetCapture(hwnd);
            int mouse_x = LOWORD(lParam);
            int mouse_y = HIWORD(lParam);
            HDC hdc = GetDC(hwnd);
            editor->cursor.pos = get_cursor_fr_mouse(hwnd, hdc, editor, mouse_x, mouse_y);
            editor->cursor.anchor = editor->cursor.pos;
            editor->cursor.selecting = 1;
            change_prefer_col(editor);
            ReleaseDC(hwnd, hdc);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (editor->cursor.selecting) {
                int mouse_x = LOWORD(lParam);
                int mouse_y = HIWORD(lParam);
                HDC hdc = GetDC(hwnd);
                editor->cursor.pos = get_cursor_fr_mouse(hwnd, hdc, editor, mouse_x, mouse_y);
                ReleaseDC(hwnd, hdc);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            editor->cursor.selecting = 0;
            ReleaseCapture();
            return 0;
        }

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ---- Main ----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Editor editor;
    if (!buffer_init(&editor.buffer)) return 1;
    cursor_init(&editor.cursor);
    editor.scroll_y = 0;
    editor.filename[0] = '\0';
    history_init(&editor.history);
    save_state(&editor);
    WNDCLASS wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Tedt";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "Tedt",
        "Text Editor",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        hInstance,
        &editor
    );
    update_scrollbar(hwnd, &editor);
    if (hwnd == NULL) {
        MessageBox(NULL, "CreateWindow failed!", "Error", MB_OK);
        return 1;
    }
    update_scrollbar(hwnd, &editor);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    free(editor.buffer.data);
    free_history(&editor.history);
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

void cursor_init(Cursor *cursor) {
    cursor->pos = 0;
    cursor->prefer_col = 0;
    cursor->anchor = 0;
    cursor->selecting = 0;
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

void get_cursor_screen_pos(HWND hwnd, HDC hdc, Editor *editor, int *x, int *y) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    int max_x = rect.right - 10;

    *x = 10;
    *y = 10 - editor->scroll_y * 20;

    for (int i = 0; i < editor->cursor.pos; i++) {
        if (editor->buffer.data[i] == '\n') {
            *x = 10;
            *y += 20;
            continue;
        }
        SIZE size;
        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);

        if (i == 0 || editor->buffer.data[i - 1] == ' ' || editor->buffer.data[i - 1] == '\n') {

            int word_end = get_word_end(editor, i);

            int word_width = 0;

            for (int j = i; j < word_end; j++) {
                SIZE s;
                GetTextExtentPoint32A(hdc, &editor->buffer.data[j], 1, &s);
                word_width += s.cx;
            }

            if (*x != 10 &&
                *x + word_width > max_x) {

                *x = 10;
                *y += 20;
            }
        }
        if (*x + size.cx > max_x) {
            *x = 10;
            *y += 20;
        }

        *x += size.cx;
    }
}

void change_prefer_col(Editor *editor) {
    int line, col;
    get_cursor_pos(editor, &line, &col);
    editor->cursor.prefer_col = col;
}

int get_cursor_fr_mouse(HWND hwnd, HDC hdc, Editor *editor, int mouse_x, int mouse_y) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    int max_x = rect.right - 10;

    int x = 10;
    int y = 10 - editor->scroll_y * 20;

    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {

            if (mouse_y >= y &&
                mouse_y < y + 20) {

                return i;
            }

            x = 10;
            y += 20;

            continue;
        }
        SIZE size;

        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);

        if (i == 0 || editor->buffer.data[i - 1] == ' ' || editor->buffer.data[i - 1] == '\n') {

            int word_end = get_word_end(editor, i);

            int word_width = 0;

            for (int j = i; j < word_end; j++) {
                SIZE s;
                GetTextExtentPoint32A(hdc, &editor->buffer.data[j], 1, &s);
                word_width += s.cx;
            }

            if (x != 10 && x + word_width > max_x) {
                x = 10;
                y += 20;
            }
        }

        if (x + size.cx > max_x) {
            x = 10;
            y += 20;
        }

        if (mouse_y >= y && mouse_y < y + 20) {
            if (mouse_x < x + size.cx / 2) return i;
            return i + 1;
        }

        x += size.cx;
    }

    if (mouse_y >= y && mouse_y < y + 20) return editor->buffer.length;
    return editor->buffer.length;
}

int is_mouse_on_text(Editor *editor, int mouse_x, int mouse_y)
{
    int y = 10 - editor->scroll_y * 20;
    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {
            y += 20;
            continue;
        }
        if (mouse_y >= y && mouse_y < y + 20) {
            return 1;
        }
    }
    return 0;
}

int get_line_count(Editor *editor) {
    int count = 1;
    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') count++;
    }
    return count;
}

int get_word_end(Editor *editor, int start) {
    int i = start;
    while (i < editor->buffer.length &&
           editor->buffer.data[i] != ' ' &&
           editor->buffer.data[i] != '\n' &&
           editor->buffer.data[i] != '\t') {
        i++;
    }
    return i;
}

void update_scrollbar(HWND hwnd, Editor *editor) {
    int lines = get_line_count(editor);

    RECT rect;
    GetClientRect(hwnd, &rect);

    int visible_lines = rect.bottom / 20;

    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = lines;
    si.nPage = visible_lines;

    int max_scroll = lines - visible_lines;
    if (max_scroll < 0) max_scroll = 0;
    if (editor->scroll_y > max_scroll) editor->scroll_y = max_scroll;
    si.nPos = editor->scroll_y;

    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

void ensure_cursor_visible(HWND hwnd, Editor *editor) {
    int line, col;
    get_cursor_pos(editor, &line, &col);

    RECT rect;
    GetClientRect(hwnd, &rect);

    int visible_lines = rect.bottom / 20;

    if (line < editor->scroll_y) {
        editor->scroll_y = line;
    }
    else if (line >= editor->scroll_y + visible_lines) {
        editor->scroll_y = line - visible_lines + 1;
    }

    update_scrollbar(hwnd, editor);
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
    save_state(editor);
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

void cursor_up(Editor *editor, int selecting) {
    int cur_start = editor->cursor.pos;
    while (cur_start > 0 && editor->buffer.data[cur_start - 1] != '\n') cur_start--;
    if (cur_start == 0) return;
    int prev_end = cur_start - 1;

    int prev_start = prev_end;
    while (prev_start > 0 && editor->buffer.data[prev_start - 1] != '\n') prev_start--;

    int prev_length = prev_end - prev_start;
    
    int new_col = editor->cursor.prefer_col;
    if (new_col > prev_length) new_col = prev_length;
    editor->cursor.pos = prev_start + new_col;
    if (!selecting) {
        editor->cursor.anchor = editor->cursor.pos;
    }
}

void cursor_down(Editor *editor, int selecting) {
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
    if (!selecting) {
        editor->cursor.anchor = editor->cursor.pos;
    }
}

void delete_button(Editor *editor) {
    if (has_selection(editor)) {
        delete_selection(editor);
        return;
    }
    if (editor->cursor.pos >= editor->buffer.length) return;
    memmove(&editor->buffer.data[editor->cursor.pos], &editor->buffer.data[editor->cursor.pos + 1], editor->buffer.length - editor->cursor.pos);
    editor->buffer.length--;
    save_state(editor);
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
    save_state(editor);
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
        save_state(editor);
    }
}

void editor_render(HWND hwnd, HDC hdc, Editor *editor) {
    SetBkMode(hdc, TRANSPARENT);

    RECT rect;
    GetClientRect(hwnd, &rect);

    int max_x = rect.right - 10;

    int x = 10;
    int y = 10 - editor->scroll_y * 20;

    int start = editor->cursor.anchor < editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;

    int end = editor->cursor.anchor > editor->cursor.pos ? editor->cursor.anchor : editor->cursor.pos;

    HBRUSH hbr = CreateSolidBrush(
        GetSysColor(COLOR_HIGHLIGHT)
    );

    for (int i = 0; i < editor->buffer.length; i++) {
        if (editor->buffer.data[i] == '\n') {
            x = 10;
            y += 20;
            continue;
        }

        SIZE size;
        GetTextExtentPoint32A(hdc, &editor->buffer.data[i], 1, &size);

        if (i == 0 ||
            editor->buffer.data[i - 1] == ' ' ||
            editor->buffer.data[i - 1] == '\n') {

            int word_end = get_word_end(editor, i);

            int word_width = 0;

            for (int j = i; j < word_end; j++) {
                SIZE s;
                GetTextExtentPoint32A(hdc, &editor->buffer.data[j], 1, &s);
                word_width += s.cx;
            }

            if (x != 10 && x + word_width > max_x) {
                x = 10;
                y += 20;
            }
        }
        if (x + size.cx > max_x) {
            x = 10;
            y += 20;
        }
        if (i >= start && i < end) {

            RECT highlight = {x, y, x + size.cx, y + 20};

            FillRect(hdc, &highlight, hbr);
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        TextOutA(hdc, x, y, &editor->buffer.data[i], 1);

        x += size.cx;
    }

    DeleteObject(hbr);
    get_cursor_screen_pos(hwnd, hdc, editor, &x, &y);
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
    save_state(editor);
}

void history_init(History *history) {
    history->states = malloc(sizeof(EditorState) * 10);
    if (history->states == NULL) {
        printf("Memory allocation failed!");
        exit(1);
    }
    history->count = 0;
    history->capacity = 10;
    history->current = -1;
}

void free_history(History *history) {
    for (int i = 0; i < history->count; i++) {
        free(history->states[i].data);
    }
    free(history->states);
}

void save_state(Editor *editor) {
    if (editor->history.current < editor->history.count - 1) {
        for (int i = editor->history.current + 1; i < editor->history.count; i++) {
            free(editor->history.states[i].data);
        }
        editor->history.count = editor->history.current + 1;
    }

    if (editor->history.count >= editor->history.capacity) {
        editor->history.capacity *= 2;
        editor->history.states = realloc(editor->history.states, sizeof(EditorState) * editor->history.capacity);
    }

    EditorState *state = &editor->history.states[editor->history.count];
    state->length = editor->buffer.length;
    state->data = malloc(state->length + 1);
    memcpy(state->data, editor->buffer.data, state->length + 1);
    state->cursor_pos = editor->cursor.pos;

    editor->history.current++;
    editor->history.count++;
}

void undo(Editor *editor) {
    if (editor->history.current <= 0) return;

    editor->history.current--;
    EditorState *state = &editor->history.states[editor->history.current];

    clear_buffer(&editor->buffer);
    memcpy(editor->buffer.data, state->data, state->length + 1);
    editor->buffer.length = state->length;
    editor->cursor.pos = state->cursor_pos;
    editor->cursor.anchor = state->cursor_pos;
    change_prefer_col(editor);
}

void redo(Editor *editor) {
    if (editor->history.current >= editor->history.count - 1) return;

    editor->history.current++;
    EditorState *state = &editor->history.states[editor->history.current];

    clear_buffer(&editor->buffer);
    buffer_insert(&editor->buffer, 0, '\0');
    memcpy(editor->buffer.data, state->data, state->length + 1);
    editor->buffer.length = state->length;
    editor->cursor.pos = state->cursor_pos;
    editor->cursor.anchor = state->cursor_pos;
    change_prefer_col(editor);
}