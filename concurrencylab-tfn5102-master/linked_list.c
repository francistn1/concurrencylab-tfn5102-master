#include <stdlib.h>
#include "linked_list.h"

/*
// Initializes a singly linked list
// If compare is NULL, the list is unsorted and new nodes are inserted at the head of the list
// If compare is not NULL, the list is sorted in increasing order based on the comparison function
void linked_list_init(linked_list_t* list, compare_fn compare)
{
    list->compare = compare;
    list->head = NULL;
}

// Inserts a node into the linked list based on the list comparison function
void linked_list_insert(linked_list_t* list, linked_list_node_t* node)
{
   // If the list is empty, insert the node as the head
    if (list->head == NULL) {
        node->next = NULL;
        list->head = node;
        return;
    }

    // Find the appropriate position to insert the new node
    linked_list_node_t* current = list->head;
    linked_list_node_t* prev = NULL;

    // Check if the new node should be inserted at the beginning
    if (list->compare && list->compare(node->shape, current->shape) < 0) {
        node->next = current;
        list->head = node;
        return;
    }

    // Traverse the list to find the appropriate position for insertion
    while (current != NULL && list->compare && list->compare(node->shape, current->shape) >= 0) {
        prev = current;
        current = current->next;
    }

    // Insert the new node
    node->next = current;
    if (prev != NULL) {
        prev->next = node;
    } else {
        list->head = node; // If prev is NULL, the new node is inserted at the beginning
    }

}


// Removes all nodes from the linked list containing the given shape
void linked_list_remove(linked_list_t* list, shape_t* shape)
{
    linked_list_node_t* current = list->head;
    linked_list_node_t* prev = NULL;

    while (current != NULL) {
        if (current->shape == shape) {
            if (prev == NULL) {
                // If the node to be removed is the head
                list->head = current->next;
            } else {
                // If the node to be removed is not the head
                prev->next = current->next;
            }

            // Update the previous node pointer
            if (prev == NULL) {
                // If the removed node was the head, update the head to the next node
                list->head = current->next;
            } else {
                // If the removed node was not the head, update the next pointer of the previous node
                prev->next = current->next;
            }

            // Move to the next node without deallocating memory
            current = current->next;

            continue;
        }

        prev = current;
        current = current->next;
    }
    
}
*/
// Creates and returns a new list
list_t* list_create()
{
    list_t* list = (list_t*)malloc(sizeof(list_t));
    if (list != NULL) {
        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
    }
    return list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    //Need to free the current node as well, root of overwritting I think
    list_node_t* current = list->head;
    while (current != NULL) {
        list_node_t* next = current->next;
        free(current);
        current = next;
    }

    free(list);
    
}

// Returns head of the list
list_node_t* list_head(list_t* list)
{
    if(list->head == NULL)
    {
        return NULL;
    }
    return list->head;
}

// Returns tail of the list
list_node_t* list_tail(list_t* list)
{
    return list->tail;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    return node->next;
}

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node)
{
    return node->prev;
}

// Returns end of the list marker
list_node_t* list_end(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return NULL;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
      if (!list)
      {
        return NULL;
      }

    list_node_t* new_node = list->head;
    while(data != NULL)
    {
        if(new_node->data == data)
        {
            return new_node;
        }
        new_node=new_node->next;//Move to next node
    }
    return NULL; // Couldn't be found
}

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data)
{
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t));
     
    if(!new_node)
    {
        return NULL;
    }
    new_node->data = data;
    // If the list is empty, insert the node as the head and tail
    if (list->head == NULL) {
        new_node->next = NULL;// This was missing and thus causing segfaults
        new_node->prev = NULL;
        list->head = new_node;
        list->tail = new_node; // Tail needs to have something 
    }
    else{
        // Insert at the end
        new_node->next = NULL;
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }

    list->count++;

    return new_node;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    if (list == NULL || node == NULL) {
        // Invalid parameters
        return;
    }

    /*
    if (node == list->head) {
        list->head = node->next;

    } else if (node == list->tail) {
        list->tail = node->prev;
    }
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;
        */
     // Update pointers of neighboring nodes
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        // Node is the head of the list
        list->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        // Node is the tail of the list
        list->tail = node->prev;
    }

    list->count--;
    free(node);
    
    /*
    if (list->count == 0) {
        // If the list is now empty, set head and tail to NULL
        list->head = NULL;
        list->tail = NULL;
    }
    */
    
  
}
