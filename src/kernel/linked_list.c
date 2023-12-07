#include "linked_list.h"

void linked_list_add(linked_list *list, linked_list_entry *entry) {
    entry->before = list->tail;
    entry->after = null;
    list->tail->after = entry;

    list->tail = entry;
}

void linked_list_add_front(linked_list *list, linked_list_entry *entry) {
    entry->before = null;
    entry->after = list->head;
    list->head->before = entry;

    list->head = entry;
}

void linked_list_insert_before(linked_list_entry *entry, linked_list_entry *new_entry) {
    if (entry->before != null) {
        entry->before->after = new_entry;
    }

    new_entry->after = entry;
    new_entry->before = entry->before;
    entry->before = new_entry;
}

void linked_list_insert_after(linked_list_entry *entry, linked_list_entry *new_entry) {
    if (entry->after != null) {
        entry->after->before = new_entry;
    }

    new_entry->before = entry;
    new_entry->after = entry->after;
    entry->after = new_entry;
}

void linked_list_remove(linked_list_entry *entry) {
    entry->before->after = entry->after;
    entry->after->before = entry->before;
}

linked_list_entry *linked_list_head(linked_list *list) {
    return list->head;
}

linked_list_entry *linked_list_tail(linked_list *list) {
    return list->tail;
}

linked_list_entry *linked_list_next(linked_list_entry *entry) {
    return entry->after;
}