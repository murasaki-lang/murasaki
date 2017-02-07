#ifndef PUBLIC_MRSK_DEV_H_INCLUDE
#define PUBLIC_MRSK_DEV_H_INCLUDE
#include "MRSK.h"

typedef enum {
    MRSK_FALSE = 0,
    MRSK_TRUE = 1
} MRSK_Boolean;

typedef struct MRSK_Object_tag MRSK_Object;
typedef struct MRSK_Array_tag MRSK_Array;
typedef struct MRSK_String_tag MRSK_String;
typedef struct MRSK_LocalEnvironment_tag MRSK_LocalEnvironment;

typedef struct {
    char *name;
} MRSK_NativePointerInfo;

typedef enum {
    MRSK_BOOLEAN_VALUE = 1,
    MRSK_INT_VALUE,
    MRSK_DOUBLE_VALUE,
    MRSK_STRING_VALUE,
    MRSK_NATIVE_POINTER_VALUE,
    MRSK_NONE_VALUE,
    MRSK_ARRAY_VALUE
} MRSK_ValueType;

typedef struct {
    MRSK_NativePointerInfo *info;
    void *pointer;
} MRSK_NativePointer;

typedef struct {
    MRSK_ValueType type;
    union {
        MRSK_Boolean boolean_value;
        int int_value;
        double double_value;
        MRSK_NativePointer native_pointer;
        MRSK_Object *object;
    } u;
} MRSK_Value;

typedef MRSK_Value MRSK_NativeFunctionProc(MRSK_Interpreter *interpreter,
                                           MRSK_LocalEnvironment *env,
                                           int arg_count, MRSK_Value *args);

void MRSK_add_nativefunction(MRSK_Interpreter *interpreter,
                             char *name, MRSK_NativeFunctionProc *proc);
void MRSK_add_global_variable(MRSK_Interpreter *inter,
                              char *identifier, MRSK_Value *value);
MRSK_Object * MRSK_create_murasaki_string(MRSK_Interpreter *inter,
                                          MRSK_LocalEnvironment *env,
                                          char *str);
MRSK_Object * MRSK_create_array(MRSK_Interpreter *inter,
                                MRSK_LocalEnvironment *env,
                                int size);

char * MRSK_value_to_string(MRSK_Value *value);

#endif
