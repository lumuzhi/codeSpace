#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

typedef struct {
    Node* top;
} Stack;

void stackInit(Stack* stack) {
    stack->top = NULL;
}

int stackIsEmpty(Stack* stack) {
    return (stack->top == NULL);
}

void stackPush(Stack* stack, int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = stack->top;
    stack->top = newNode;
}

int stackPop(Stack* stack) {
    if (stackIsEmpty(stack)) {
        printf("Stack is empty.\n");
        return -1;
    }

    int data = stack->top->data;
    Node* temp = stack->top;
    stack->top = stack->top->next;
    free(temp);

    return data;
}

void stackPrint(Stack* stack) {
    if (stackIsEmpty(stack)) {
        printf("Stack is empty.\n");
        return;
    }

    Node* current = stack->top;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

void stackDestroy(Stack* stack) {
    while (!stackIsEmpty(stack)) {
        stackPop(stack);
    }
}

int main() {
    Stack stack;
    stackInit(&stack);

    stackPush(&stack, 1);
    stackPush(&stack, 2);
    stackPush(&stack, 3);

    printf("Stack: ");
    stackPrint(&stack);

    int poppedValue = stackPop(&stack);
    printf("Popped value: %d\n", poppedValue);

    printf("Stack after pop: ");
    stackPrint(&stack);

    stackDestroy(&stack);
    stackPrint(&stack);

    return 0;
}
