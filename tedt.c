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
    int position;
} Cursor;

int buffer_init(Buffer *buffer);
void buffer_backspace(Buffer *buffer, int pos);
void buffer_print(Buffer *buffer);
void buffer_insert(Buffer *buffer, int pos, char c);

int main() {
    printf("Tedt\n---------------------------------------------------------\n");
    Buffer buffer; 
    if (!buffer_init(&buffer)) return 1;
    Cursor cursor;
    cursor.position = buffer.length;
    while(1) {
        int c = _getch();
        if (c == ESC) break;
        if (c == BACKSPACE) {
            buffer_backspace(&buffer, cursor.position);
            printf("\b \b");
            if (cursor.position != 0) cursor.position--;
            continue;
        }
        if (c == ENTER) {
            buffer_insert(&buffer, cursor.position, '\n');
            cursor.position++;
            printf("\n");
            continue;
        }
        if (c == SPECIAL) {
            int key = _getch();
            if (key == LEFT) {
                if (cursor.position == 0) continue;
                cursor.position--;
                continue;
            }
            if (key == RIGHT) {
                if (cursor.position == buffer.length) continue;
                cursor.position++;
                continue;
            }
            continue;
        }

        buffer_insert(&buffer, cursor.position, c);
        cursor.position++;
        printf("%c", c);
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
    if (buffer->length == 0) return;
    memmove(&buffer->data[pos-1], &buffer->data[pos], buffer->length - pos + 1);
    buffer->length--;
}

void buffer_print(Buffer *buffer) {
    printf("\nBuffer:\n");
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