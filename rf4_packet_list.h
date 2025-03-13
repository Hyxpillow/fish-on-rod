#include <stdio.h>
#include <stdlib.h>

// Define the structure for each node in the list

typedef struct Node {
    u_int seq;
    u_int size;
    u_char* buffer;             // Data stored in the node
    struct Node* next;    // Pointer to the next node
} Node;

// Define the list structure
typedef struct {
    Node* head;           // Pointer to the first node in the list
    int count;            // Number of nodes in the list
} List;

// Initialize a new empty list
List* list_create() {
    List* list = (List*)malloc(sizeof(List));
    if (list == NULL) {
        perror("Failed to allocate memory for list");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->count = 0;
    return list;
}

// Insert a new node at the beginning of the list
void list_insert(List* list, u_int seq, u_int size, u_char* src) {
    if (list == NULL) {
        return;
    }

    // Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node");
        return;
    }
    u_char* buffer = (u_char*)malloc(size);
    if (buffer == NULL) {
        perror("Failed to allocate memory for new buffer");
        return;
    }
    memcpy(buffer, src, size);
    new_node->seq = seq;
    new_node->size = size;
    new_node->buffer = buffer;
    new_node->next = list->head;
    
    // Update the list's head pointer
    list->head = new_node;
    
    // Increment the count
    list->count++;
}

// Delete the first occurrence of a value from the list
void list_delete(List* list, Node* node) {
    if (list == NULL || list->head == NULL) {
        return;
    }

    if (list->head == node) {
        list->head = node->next;
        list->count--;
        free(node->buffer);
        free(node);
        return;
    }

    // Search for the node to delete
    Node* current = list->head;
    while (current->next != NULL && current->next != node) {
        current = current->next;
    }

    // If found, delete it
    if (current->next != NULL) {
        Node* to_delete = current->next;
        current->next = current->next->next;
        free(to_delete->buffer);
        free(to_delete);
        list->count--;
    }
}

// Search for a value in the list
// Returns 1 if found, 0 if not found
Node* list_search(List* list, int seq) {
    if (list == NULL || list->count == 0) {
        return NULL;
    }

    Node* current = list->head;
    while (current != NULL) {
        if (current->seq == seq) {
            return current;  // Found
        }
        current = current->next;
    }

    return NULL;  // Not found
}

// Get the current number of nodes in the list
int list_size(List* list) {
    if (list == NULL) {
        return 0;
    }
    return list->count;
}

// Print the list for debugging
// Free all memory used by the list
// void list_destroy(List* list) {
//     if (list == NULL) {
//         return;
//     }

//     Node* current = list->head;
//     Node* next;

//     while (current != NULL) {
//         next = current->next;
//         free(current);
//         current = next;
//     }

//     free(list);
// }