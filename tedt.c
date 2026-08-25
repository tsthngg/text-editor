#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <windows.h>

#define ESC 27
#define BACKSPACE 8
#define ENTER 13
#define SPECIAL 224
#define LEFT 75
#define RIGHT 77
#define UP 72
#define DOWN 80
#define HOME 71
#define END 79
#define DELETE 83

typedef struct {
    char *data;
    int length;
    int cap;
} Buffer;

typedef struct {
    int pos;
    int prefer_col;
} Cursor;

int buffer_init(Buffer *buffer);
void buffer_print(Buffer *buffer);
void buffer_insert(Buffer *buffer, int pos, char c);

void backspace(Buffer *buffer, int pos);
void home(Buffer *buffer, Cursor *cursor);
void end(Buffer *buffer, Cursor *cursor);
void delete_button(Buffer *buffer, int pos);

void cursor_up(Buffer *buffer, Cursor *cursor);
void cursor_down(Buffer *buffer, Cursor *cursor);
void cursor_to_screen(HANDLE hConsole, Buffer *buffer, Cursor *cursor);
void get_cursor_pos(Buffer *buffer, Cursor *cursor, int *line, int *col);
void change_prefer_col(Buffer *buffer, Cursor *cursor);

void editor_render(HANDLE hConsole, Buffer *buffer, Cursor *cursor);
void editor_key_handle(HANDLE hConsole, Buffer *buffer, Cursor *cursor, int c);

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    Buffer buffer; 
    if (!buffer_init(&buffer)) return 1;
    
    Cursor cursor;
    cursor.pos = buffer.length;
    cursor.prefer_col = 0;
    cursor_to_screen(hConsole, &buffer, &cursor);

    while(1) {
        int c = _getch();
        if (c == ESC) break;
        editor_key_handle(hConsole, &buffer, &cursor, c);
        editor_render(hConsole, &buffer, &cursor);
    } 

    free(buffer.data);
    return 0;
}

void editor_key_handle(HANDLE hConsole, Buffer *buffer, Cursor *cursor, int c){
    if (c == BACKSPACE) {
        if (cursor->pos > 0) {
            cursor->pos--;
            backspace(&buffer, cursor->pos);
            change_prefer_col(&buffer, &cursor);
        }
    }
    else if (c == ENTER) {
        buffer_insert(&buffer, cursor->pos, '\n');
        cursor->pos++;
        change_prefer_col(&buffer, &cursor);
    }
    else if (c == SPECIAL) {
        int key = _getch();
        if (key == LEFT) {
            if (cursor->pos == 0) return;
            cursor->pos--;
            cursor_to_screen(hConsole, &buffer, &cursor);
            change_prefer_col(&buffer, &cursor);
        }
        else if (key == RIGHT) {
            if (cursor->pos == buffer->length) return;
            cursor->pos++;
            cursor_to_screen(hConsole, &buffer, &cursor);
            change_prefer_col(&buffer, &cursor);
        }
        else if (key == UP) cursor_up(&buffer, &cursor);
        else if (key == DOWN) cursor_down(&buffer, &cursor);
        else if (key == HOME) home(&buffer, &cursor);
        else if (key == END) end(&buffer, &cursor);
        else if (key == DELETE) delete_button(&buffer, cursor->pos);
    }
    else {
        buffer_insert(&buffer, cursor->pos, c);
        cursor->pos++;
        change_prefer_col(&buffer, &cursor);
    }
}

int buffer_init(Buffer *buffer) {
    buffer->data = malloc(16);
    if (buffer->data == NULL) {
        printf("Memory allocation failed!");
        return 0;
    }
    buffer->length = 0;
    buffer->cap = 16;
    return 1;
}

void backspace(Buffer *buffer, int pos) {
    if (pos == 0 || buffer->length == 0) return;
    memmove(&buffer->data[pos-1], &buffer->data[pos], buffer->length - pos + 1);
    buffer->length--;
}

void buffer_print(Buffer *buffer) {
    for (int i = 0; i < buffer->length; i++) {
        printf("%c", buffer->data[i]);
    }
}

void buffer_insert(Buffer *buffer, int pos, char c) {
    char* temp;
    if (buffer->length + 1 >= buffer->cap) {
        temp = realloc(buffer->data, buffer->cap*2);
        if (temp == NULL) {
            printf("Memory allocation failed!");
            return;
        } 
        buffer->data = temp;
        buffer->cap *= 2;
    }
    memmove(&buffer->data[pos + 1], &buffer->data[pos], buffer->length - pos + 1);
    buffer->data[pos] = c;
    buffer->length++;
}

void get_cursor_pos(Buffer *buffer, Cursor *cursor, int *line, int *col) {
    *line = *col = 0;
    for (int i = 0; i < cursor->pos; i++) {
        if (buffer->data[i] == '\n') {
            (*line)++;
            (*col) = 0;
        }
        else (*col)++;
    }
}

void cursor_to_screen(HANDLE hConsole, Buffer *buffer, Cursor *cursor) {
    int line = 0, col = 0;
    get_cursor_pos(buffer, cursor, &line, &col);
    COORD pos;
    pos.X = col;
    pos.Y = line;
    SetConsoleCursorPosition(hConsole, pos);
}

void editor_render(HANDLE hConsole, Buffer *buffer, Cursor *cursor) {
    system("cls");
    buffer_print(buffer);
    cursor_to_screen(hConsole, buffer, cursor);
}

void change_prefer_col(Buffer *buffer, Cursor *cursor) {
    int line, col;
    get_cursor_pos(buffer, cursor, &line, &col);
    cursor->prefer_col = col;
}

void cursor_up(Buffer *buffer, Cursor *cursor) {
    int cur_start = cursor->pos;
    while (cur_start > 0 && buffer->data[cur_start - 1] != '\n') cur_start--;
    if (cur_start == 0) return;
    int prev_start = cur_start - 1;
    while (prev_start > 0 && buffer->data[prev_start - 1] != '\n') prev_start--;
    
    int prev_length = 0;
    while (prev_start + prev_length < buffer->length &&
       buffer->data[prev_start + prev_length] != '\n') prev_length++;
    
    int new_col = cursor->prefer_col;
    if (new_col > prev_length) new_col = prev_length;
    cursor->pos = prev_start + new_col;
}

void cursor_down(Buffer *buffer, Cursor *cursor) {
    int cur_end = cursor->pos;
    while (cur_end < buffer->length && buffer->data[cur_end] != '\n') cur_end++;
    if (cur_end == buffer->length) return;
    int next_start = cur_end + 1;

    int next_length = 0;
    while (next_start + next_length < buffer->length &&
    buffer->data[next_start + next_length] != '\n') next_length++;
    
    int new_col = cursor->prefer_col;
    if (new_col > next_length) new_col = next_length;
    cursor->pos = next_start + new_col;
}

void home(Buffer *buffer, Cursor *cursor) {
    int start = cursor->pos;
    while (start > 0 && buffer->data[start - 1] != '\n') start--;
    cursor->pos = start;
    change_prefer_col(buffer, cursor);
}

void end(Buffer *buffer, Cursor *cursor) {
    int end = cursor->pos;
    while (end < buffer->length && buffer->data[end] != '\n') end++;
    cursor->pos = end;
    change_prefer_col(buffer, cursor);
}

void delete_button(Buffer *buffer, int pos) {
    if (pos >= buffer->length) return;
    memmove(&buffer->data[pos], &buffer->data[pos + 1], buffer->length - pos);
    buffer->length--;
}