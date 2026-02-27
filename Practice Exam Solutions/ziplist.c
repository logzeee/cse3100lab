#include <stdio.h>
#include <stdlib.h>

// Node structure
typedef struct Node {
  int data;
  struct Node *next;
} Node;

// Create a new node
Node *createNode(int data) {
  Node *newNode = (Node *)malloc(sizeof(Node));
  newNode->data = data;
  newNode->next = NULL;
  return newNode;
}

// Insert at end
void insertEnd(Node **head, int data) {
  Node *newNode = createNode(data);
  if (!*head) {
    *head = newNode;
    return;
  }
  Node *temp = *head;
  while (temp->next)
    temp = temp->next;
  temp->next = newNode;
}

// Print list
void printList(Node *head) {
  while (head) {
    printf("%d ", head->data);
    head = head->next;
  }
  printf("\n");
}

// Reverse a linked list
Node *reverseList(Node *head) {
  Node *prev = NULL;
  while (head) {
    Node *next = head->next;
    head->next = prev;
    prev = head;
    head = next;
  }
  return prev;
}

// Zip a single linked list
void zipList(Node **headRef) {
  if (!*headRef || !(*headRef)->next)
    return;

  // Find middle
  Node *t1 = *headRef, *t2 = *headRef;
  while (t2->next && t2->next->next) {
    t1 = t1->next;
    t2 = t2->next->next;
  }
	// reverse
  Node *second = reverseList(t1->next);
  t1->next = NULL;
	
	// merge
  Node *first = *headRef;
  while (second) {
    Node *tmp1 = first->next;
    Node *tmp2 = second->next;

    first->next = second;
    second->next = tmp1;

    first = tmp1;
    second = tmp2;
  }
}

int main(int argc, char *argv[]) {
  Node *head = NULL;

  if (argc < 2) {
    printf("Usage: %s <list of integers>\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    int val = atoi(argv[i]); 
    insertEnd(&head, val);
  }

  printf("Original list:\n");
  printList(head);

  zipList(&head);

  printf("Zipped list:\n");
  printList(head);

	Node *temp; 	
  while (head) { 
    temp = head; 
    head = head->next; 
    free(temp); 
  }


  return 0;
}
