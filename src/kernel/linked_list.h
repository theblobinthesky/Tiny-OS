#pragma once
#include "util.h"

struct linked_list_ {
    struct linked_list_entry_ *head;
    struct linked_list_entry_ *tail;
};

struct linked_list_entry_ {
    struct linked_list_entry_ *before;
    struct linked_list_entry_ *after;
};

typedef struct linked_list_ linked_list;
typedef struct linked_list_entry_ linked_list_entry;

void linked_list_add(linked_list *list, linked_list_entry *entry);
void linked_list_add_front(linked_list *list, linked_list_entry *entry);
void linked_list_insert_before(linked_list_entry *entry, linked_list_entry *new_entry);
void linked_list_insert_after(linked_list_entry *entry, linked_list_entry *new_entry);
void linked_list_remove(linked_list_entry *entry);

linked_list_entry *linked_list_head(linked_list *list);
linked_list_entry *linked_list_tail(linked_list *list);
linked_list_entry* linked_list_next(linked_list_entry *entry);