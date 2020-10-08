#ifndef _HELPER_H
#define _HELPER_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UID_LENGTH 5
#define PASS_LENGTH 8
#define FALSE 0
#define TRUE 1

typedef struct element {
    char *key, *value;
    struct element *next;
} Element;

typedef struct map {
    int size;
    Element *elements;
} Map;

char *set(char *string);
int length(char *string);
Map *newMap();
void put(Map *myMap, char *key, char *value);
char *get(Map *myMap, char *key);
void print(Map *myMap);
void delete(Map *myMap);
void rm(Element *element);
int validUID(char *uid);
int validPASS(char *pw);
int getWords(char *buffer);

#endif