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

typedef struct {
    char *data;
    int length;
    int cap;
} Buffer;

typedef struct {
    int pos;
} Cursor;

int buffer_init(Buffer *buffer);
void buffer_backspace(Buffer *buffer, int pos);
void buffer_print(Buffer *buffer);
void buffer_insert(Buffer *buffer, int pos, char c);
void cursor_to_screen(HANDLE hConsole, Buffer *buffer, Cursor *cursor);
void editor_render(HANDLE hConsole, Buffer *buffer, Cursor *cursor);

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    Buffer buffer; 
    if (!buffer_init(&buffer)) return 1;
    
    Cursor cursor;
    cursor.pos = buffer.length;
    cursor_to_screen(hConsole, &buffer, &cursor);

    while(1) {
        int c = _getch();
        if (c == ESC) break;
        else if (c == BACKSPACE) {
            buffer_backspace(&buffer, cursor.pos);
            printf("\b \b");
            if (cursor.pos != 0) cursor.pos--;
        }
        else if (c == ENTER) {
            buffer_insert(&buffer, cursor.pos, '\n');
            cursor.pos++;
            printf("\n");
        }
        else if (c == SPECIAL) {
            int key = _getch();
            if (key == LEFT) {
                if (cursor.pos == 0) continue;
                cursor.pos--;
                cursor_to_screen(hConsole, &buffer, &cursor);
            }
            if (key == RIGHT) {
                if (cursor.pos == buffer.length) continue;
                cursor.pos++;
                cursor_to_screen(hConsole, &buffer, &cursor);
            }
        }
        else {
            buffer_insert(&buffer, cursor.pos, c);
            cursor.pos++;
        }
        editor_render(hConsole, &buffer, &cursor);
    } 

    buffer_print(&buffer);
    free(buffer.data);
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
    return 1;
}

void buffer_backspace(Buffer *buffer, int pos) {
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

void cursor_to_screen(HANDLE hConsole, Buffer *buffer, Cursor *cursor) {
    int x = 0, y = 0;
    for (int i = 0; i < cursor->pos; i++) {
        if (buffer->data[i] == '\n') {
            y++;
            x = 0;
        }
        else x++;
    }
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(hConsole, pos);
}

void editor_render(HANDLE hConsole, Buffer *buffer, Cursor *cursor) {
    system("cls");
    buffer_print(buffer);
    cursor_to_screen(hConsole, buffer, cursor);
}
