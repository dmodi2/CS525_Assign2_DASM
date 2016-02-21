#include<stdio.h>
#include<stdlib.h>

typedef struct list{
    int data;
    struct list *next;
  }listT;

void push(listT** head_ref, int data);
void printlist(listT* node);

int main(){

  listT *node = NULL;
  push(&node, 5);
  push(&node, 10);
  push(&node, 15);
  
  printlist(node);
  return 0;
}
  
void push(listT** head_ref, int data){
    listT* node = (listT *)malloc(sizeof(listT));
    node->data = data;
    node->next = (*head_ref);
    (*head_ref) = node;
}

void printlist(listT* node){
    while(node != NULL){
      printf("%d\n", node->data);
      node = node->next;
} 
}

