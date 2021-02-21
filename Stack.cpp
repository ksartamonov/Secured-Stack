#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#define case_of_switch(en) case en: return #en;

//------------------------------------------------------------------------------

#define POISON NAN

#define ELEMENT_FORMAT "%lf"

#define STRUCT_CANNARY_VALUE 0xFFFFFFFFFFFFFFFF
#define STRUCT_CANNARY_FORMAT "%08X"
#define DATA_CANNARY_VALUE 2.0
#define DATA_CANNARY_FORMAT "%lf"
#define MAXSIZE 1000

typedef double StackElement;

//------------------------------------------------------------------------------

enum ERROR{

    OK,
    CREATION_FAILED_NOT_ENOUGH_MEMORY,
    CREATION_FAILED_ALREADY_EXISTS,
    STACK_ALREADY_DEAD,
    STACK_PTR_UNAVAILABLE,
    DATA_PTR_UNAVAILABLE,
    STACK_PTR_EQUALS_DATA_PTR,
    CAPACITY_LESS_ONE,
    CAPACITY_LESS_MINIMAL,
    SIZE_LESS_ZERO,
    SIZE_GREATER_THAN_CAPACITY,
    DATA_CORRUPTED_CONTAINS_POISON,
    DATA_CORUPTED_UNEXPECTED_NON_POISON_VALUE_ABOVE_TOP,
    DATA_CORRUPTED_CANNARY_IS_DEAD,
    STRUCT_CORRUPTED_CANNARY_IS_DEAD,
    DATA_CORRUPTED_WRONG_HASH,
    STRUCT_CORRUPTED_WRONG_HASH,
    PTR_UNAVAILABLE
};

struct Stack{

    long int CANNARY1 = 0;
    int minimal_capacity = 0;
    int capacity = 0;
    int stack_size = 0;
    StackElement* data = nullptr;
    int struct_hash = 0;
    int data_hash = 0;
    long int CANNARY2 = 0;
    int EXISTS = 0;
};


void CHECK(Stack* stk);
void Stack_Dump(Stack* stk, ERROR status, FILE* stream);
int Pointer_OK(void* ptr);
int Stack_Pop(Stack* stk, StackElement* popped);
int Stack_Push(Stack* stk, StackElement value);
int ResizeDown(Stack* stk);
int ResizeUp(Stack* stk);
void BYE_BYE(Stack* stk, ERROR error);
void DEATH(Stack* stk, ERROR error);
ERROR StackOK(Stack* stk);
ERROR StackConstructor(Stack* stk, int capacity);
int IsPoison(StackElement value);
int RESIZE_DOWN_HELPER(Stack* stk);
int Stack_CannaryAlive(Stack* stk);
int Data_CannaryAlive(Stack* stk);
StackElement* CannaryRealloc(Stack* stk, int newsize);
StackElement* CannaryCalloc(Stack* stk, int capacity);
ERROR Stack_First_Init(int capacity, Stack** new_stack_pointer_adress);
void StackPrint(Stack* stk, ERROR status, FILE* stream);
int ComputeHash(char* bufferr, size_t bytes);
void RecomputeHashes(Stack* stk);


//------------------------------------------------------------------------------
static const char* getERRORName(ERROR error)
{

    switch(error)
    {
        case_of_switch(OK)
        case_of_switch(CREATION_FAILED_NOT_ENOUGH_MEMORY)
        case_of_switch(CREATION_FAILED_ALREADY_EXISTS)
        case_of_switch(STACK_PTR_UNAVAILABLE)
        case_of_switch(DATA_PTR_UNAVAILABLE)
        case_of_switch(STACK_PTR_EQUALS_DATA_PTR)
        case_of_switch(CAPACITY_LESS_ONE)
        case_of_switch(CAPACITY_LESS_MINIMAL)
        case_of_switch(SIZE_LESS_ZERO)
        case_of_switch(SIZE_GREATER_THAN_CAPACITY)
        case_of_switch(DATA_CORRUPTED_CONTAINS_POISON)
        case_of_switch(DATA_CORUPTED_UNEXPECTED_NON_POISON_VALUE_ABOVE_TOP)
        case_of_switch(DATA_CORRUPTED_CANNARY_IS_DEAD)
        case_of_switch(STRUCT_CORRUPTED_CANNARY_IS_DEAD)
        case_of_switch(DATA_CORRUPTED_WRONG_HASH)
        case_of_switch(STRUCT_CORRUPTED_WRONG_HASH)
    }
}
//------------------------------------------------------------------------------


ERROR Stack_First_Init(int capacity, Stack** new_stack_pointer_adress)
{

    if (!Pointer_OK(new_stack_pointer_adress)) return PTR_UNAVAILABLE;

    Stack* stk = (Stack*)calloc(1, sizeof(Stack));

    if (!Pointer_OK(stk)) return CREATION_FAILED_NOT_ENOUGH_MEMORY;

    ERROR status = StackConstructor(stk, capacity);
    if (status == OK) *new_stack_pointer_adress = stk;
    return status;
}

//------------------------------------------------------------------------------

ERROR StackConstructor(Stack* stk, int capacity)
{
    if (capacity == 0) capacity = 1;
    if (!Pointer_OK(stk)) return STACK_PTR_UNAVAILABLE;
    if (capacity < 1) return CAPACITY_LESS_ONE;
    if (stk -> EXISTS) return CREATION_FAILED_ALREADY_EXISTS;

    stk -> EXISTS  = 1;
    stk -> CANNARY1 = STRUCT_CANNARY_VALUE;
    stk -> minimal_capacity = capacity;
    stk -> capacity = capacity;
    stk -> stack_size = 0;
    if(!Pointer_OK(CannaryCalloc(stk, capacity))) return CREATION_FAILED_NOT_ENOUGH_MEMORY;
    stk -> CANNARY2 = STRUCT_CANNARY_VALUE;
    for (int i = 0; i < capacity;  (stk -> data)[i++]  = POISON);
    RecomputeHashes(stk);
    return StackOK(stk);
}

//------------------------------------------------------------------------------

StackElement* CannaryCalloc(Stack* stk, int capacity)
{

    stk -> data = (StackElement*)calloc(1, (capacity + 2) * sizeof(StackElement));
    if (!Pointer_OK(stk -> data)) return nullptr;
    *(stk -> data) = DATA_CANNARY_VALUE;
    (stk -> data)[capacity + 1] = DATA_CANNARY_VALUE;
    stk -> data = (StackElement*)((StackElement*)(stk -> data) + 1);
    return stk -> data;
}

//------------------------------------------------------------------------------

StackElement* CannaryRealloc(Stack* stk, int newsize)
{

    StackElement* resized = (StackElement*)realloc((StackElement*)(stk -> data) - 1, newsize + 2 * sizeof(StackElement));

    if (Pointer_OK(resized)) return (StackElement*)((StackElement*)resized + 1);

    return nullptr;
}

//------------------------------------------------------------------------------

int ResizeUp(Stack* stk)
{

    CHECK(stk);

    StackElement* resized = CannaryRealloc(stk, int((stk -> capacity) * sizeof(StackElement) * 2));

    if (Pointer_OK(resized)){
        stk -> capacity = stk -> capacity * 2;
        stk -> data = resized;
        (stk -> data)[stk -> capacity] = DATA_CANNARY_VALUE;

        for (int i = (stk -> stack_size); i < (stk -> capacity); (stk -> data)[i++] = POISON);

        return 1;
    }

    CHECK(stk);
    return 0;
}

//------------------------------------------------------------------------------

int ResizeDown(Stack* stk)
{

    CHECK(stk);

    StackElement* resized = CannaryRealloc(stk, (int(sizeof(StackElement) * (stk -> capacity) / 2)));

    if (Pointer_OK(resized)){
        stk -> capacity = int(stk -> capacity / 2);
        stk -> data = resized;
        (stk -> data)[stk -> capacity] = DATA_CANNARY_VALUE;
        return 1;
    }

    RecomputeHashes(stk);
    CHECK(stk);
    return 0;
}

//------------------------------------------------------------------------------

int RESIZE_DOWN_HELPER(Stack* stk)
{
    return stk -> minimal_capacity - 1;

}

//------------------------------------------------------------------------------

int Stack_Push(Stack* stk, StackElement value)
{

    CHECK(stk);

    if (value == POISON) return 0;

    if (stk -> stack_size == stk -> capacity){
        if(!ResizeUp(stk)) return 0;
    }

    if (!IsPoison((stk -> data)[stk -> stack_size])) BYE_BYE(stk, DATA_CORUPTED_UNEXPECTED_NON_POISON_VALUE_ABOVE_TOP);
    stk -> data [stk -> stack_size] = value;
    stk -> stack_size++;

    RecomputeHashes(stk);
    CHECK(stk);
    return 1;
}

//------------------------------------------------------------------------------

StackElement Stack_Pop(Stack* stk, int* status_ptr)
{

    CHECK(stk);
    if (stk -> stack_size == 0){
        *status_ptr = 0;
        return 0;
    }
    if (stk -> stack_size + (RESIZE_DOWN_HELPER(stk)) < int((stk -> capacity / 2) + 1) && stk -> capacity != stk -> minimal_capacity)
        ResizeDown(stk);

    StackElement last = stk -> data [stk -> stack_size - 1];
    stk -> data [stk -> stack_size - 1] = POISON;
    (stk -> stack_size)--;

    RecomputeHashes(stk);
    CHECK(stk);
    return last;
}

//------------------------------------------------------------------------------

void Stack_Dump(Stack* stk, ERROR status)
{

    FILE* fp = fopen("log.txt", "w");
    StackPrint(stk, status, fp);
    fclose(fp);
}

//------------------------------------------------------------------------------

void StackPrint(Stack* stk, ERROR status, FILE* stream)
{

    fprintf(stream, "Stack(%s)[%p]:\n    CANNARY1: |" STRUCT_CANNARY_FORMAT "|,\n    capacity: |%d|,\n    stack_size: |%d|,\n"
    "    stack_hash: |%X|,\n    data_hash: |%X|,\n    CANNARY2: |" STRUCT_CANNARY_FORMAT "|,\n    data[%p]: {\n",
    getERRORName(status), stk, stk -> CANNARY1, stk -> capacity, stk -> stack_size,
    stk -> struct_hash, stk -> data_hash, stk -> CANNARY2, stk -> data);
    if (Pointer_OK(stk -> data)){
        fprintf(stream, "      CANNARY1: " DATA_CANNARY_FORMAT ",\n", (stk -> data)[-1]);
        for(int i = 0; i < (stk -> capacity); fprintf(stream, "      " ELEMENT_FORMAT ",\n", (stk -> data)[i++]));
        fprintf(stream, "      CANNARY2: " DATA_CANNARY_FORMAT ",\n", (stk -> data)[stk -> capacity]);
    }
    fprintf(stream, "    }\n");
}

//------------------------------------------------------------------------------

void CHECK(Stack* stk)
{

    ERROR status = StackOK(stk);
    switch(status){

    case OK: return;
    case STACK_PTR_UNAVAILABLE: DEATH(stk, status);
    default: BYE_BYE(stk, status);
    }
}

//------------------------------------------------------------------------------

int Pointer_OK(void* ptr)
{
    if (!ptr) return 0;
    return 1;
}

//------------------------------------------------------------------------------

int Stack_CannaryAlive(Stack* stk){

    if (stk -> CANNARY1 == STRUCT_CANNARY_VALUE && stk -> CANNARY2 == STRUCT_CANNARY_VALUE) return 1;
    return 0;
}

//------------------------------------------------------------------------------

int Data_CannaryAlive(Stack* stk)
{
    if ((stk -> data)[-1] == DATA_CANNARY_VALUE && (stk -> data)[stk -> capacity] == DATA_CANNARY_VALUE) return 1;
    return 0;
}

//------------------------------------------------------------------------------

ERROR StackOK(Stack* stk)
{

    if (!Pointer_OK(stk))                             return STACK_PTR_UNAVAILABLE;
    if (!Pointer_OK(stk -> data))                     return DATA_PTR_UNAVAILABLE;
    if (((void*)stk) == ((void*)stk -> data))          return STACK_PTR_EQUALS_DATA_PTR;
    if (stk -> capacity < 1)                            return CAPACITY_LESS_ONE;
    if (stk -> capacity < stk -> minimal_capacity)     return CAPACITY_LESS_MINIMAL;
    if (stk -> stack_size < 0)                          return SIZE_LESS_ZERO;
    if (stk -> stack_size > stk -> capacity)           return SIZE_GREATER_THAN_CAPACITY;
    if (!Data_CannaryAlive(stk))                        return DATA_CORRUPTED_CANNARY_IS_DEAD;
    if (!Stack_CannaryAlive(stk))                       return STRUCT_CORRUPTED_CANNARY_IS_DEAD;

    int current_data_hash = stk -> data_hash;
    int current_struct_hash = stk -> struct_hash;
    RecomputeHashes(stk);
    if (current_data_hash != stk -> data_hash)      return DATA_CORRUPTED_WRONG_HASH;
    if (current_struct_hash != stk-> struct_hash)  return STRUCT_CORRUPTED_WRONG_HASH;
    return OK;

}

//------------------------------------------------------------------------------

void DEATH(Stack* stk, ERROR error)
{
    printf("В стеке возникла критическая ошибка[%p]: %s", stk, getERRORName(error));
    abort();
}

//------------------------------------------------------------------------------

void BYE_BYE(Stack* stk, ERROR error)
{
    printf("Возникла ошибка! Стек напечатан:\n");
    Stack_Dump(stk, error);
    abort();
}

//------------------------------------------------------------------------------

int IsPoison(StackElement value)
{
    return isnan(value);// 0xDEADBEEF
}

//------------------------------------------------------------------------------

ERROR StackDestructor(Stack* stk)
{

    CHECK(stk);

    if (!stk -> EXISTS) return STACK_ALREADY_DEAD;
    free(stk -> data);
    free(stk);
    return OK;
}

//------------------------------------------------------------------------------

ERROR StackClear(Stack* stk)
{

    CHECK(stk);

    stk -> capacity = stk -> minimal_capacity;
    stk -> stack_size = 0;
    stk -> data = CannaryRealloc(stk, stk -> capacity);

    for (int i = 0; i < stk -> capacity;  (stk -> data)[i++]  = POISON);

    RecomputeHashes(stk);
    CHECK(stk);
    return OK;

}

//------------------------------------------------------------------------------

int ComputeHash(char* buffer, size_t bytes)
{
    int sum = 0xDAFF;
    for (int i = 0; i < bytes; i++) sum = (sum * 2) + 2 * 8 *(int)buffer[i];
    return sum;
}

//------------------------------------------------------------------------------

void RecomputeHashes(Stack* stk)
{

    stk -> struct_hash = 0;
    stk -> data_hash = 0;
    stk -> struct_hash = ComputeHash((char*)(stk), sizeof(Stack));
    stk -> data_hash = ComputeHash((char*)(stk -> data), sizeof(StackElement) * (stk -> stack_size + 2));
}

//------------------------------------------------------------------------------
