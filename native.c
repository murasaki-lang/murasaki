#include "DBG.h"
#include "MRSK_dev.h"
#include "murasaki.h"

#define NATIVE_LIB_NAME "murasaki.lang.file"

static MRSK_NativePointerInfo st_native_lib_info = {
    NATIVE_LIB_NAME
};

static void check_argument_count(int arg_count, int true_count)
{
    if (arg_count < true_count) {
        mrsk_runtime_error(0, ARGUMENT_TOO_FEW_ERR, MESSAGE_ARGUMENT_END);
    } else if (arg_count > true_count) {
        mrsk_runtime_error(0, ARGUMENT_TOO_MANY_ERR, MESSAGE_ARGUMENT_END);
    }
}

MRSK_Value mrsk_nv_print_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                              int arg_count, MRSK_Value *args)
{
    MRSK_Value value;
    char *str;

    value.type = MRSK_NONE_VALUE;

    check_argument_count(arg_count, 1);
    str = MRSK_value_to_string(&args[0]);
    printf("%s", str);
    MEM_free(str);

    return value;
}

MRSK_Value mrsk_nv_fopen_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                              int arg_count, MRSK_Value *args)
{
    MRSK_Value value;
    FILE *fp;

    check_argument_count(arg_count, 2);

    if (args[0].type != MRSK_STRING_VALUE
        || args[1].type != MRSK_STRING_VALUE) {
        mrsk_runtime_error(0, FOPEN_ARGUMENT_TYPE_ERR, MESSAGE_ARGUMENT_END);
    }

    fp = fopen(args[0].u.object->u.string.string, args[1].u.object->u.string.string);

    if (fp == NULL) {
        value.type = MRSK_NONE_VALUE;
    } else {
        value.type = MRSK_NATIVE_POINTER_VALUE;
        value.u.native_pointer.info = &st_native_lib_info;
        value.u.native_pointer.pointer = fp;
    }

    return value;
}

static MRSK_Boolean check_native_pointer(MRSK_Value *value)
{
    return value->u.native_pointer.info == &st_native_lib_info;
}

MRSK_Value mrsk_nv_fclose_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                               int arg_count, MRSK_Value *args)
{
    MRSK_Value value;
    FILE *fp;

    check_argument_count(arg_count, 1);

    value.type = MRSK_NONE_VALUE;
    if (args[0].type != MRSK_NATIVE_POINTER_VALUE
        || !check_native_pointer(&args[0])) {
        mrsk_runtime_error(0, FCLOSE_ARGUMENT_TYPE_ERR, MESSAGE_ARGUMENT_END);
    }
    fp = args[0].u.native_pointer.pointer;
    fclose(fp);
    return value;
}

MRSK_Value mrsk_nv_fgets_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                             int arg_count, MRSK_Value *args)
{
    MRSK_Value value;
    FILE *fp;
    char buf[LINE_BUF_SIZE];
    char *ret_buf = NULL;
    int ret_len = 0;

    check_argument_count(arg_count, 1);

    if (args[0].type != MRSK_NATIVE_POINTER_VALUE
        || !check_native_pointer(&args[0])) {
        mrsk_runtime_error(0, FGETS_ARGUMENT_TYPE_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    fp = args[0].u.native_pointer.pointer;

    while (fgets(buf, LINE_BUF_SIZE, fp)) {
        int new_len;
        new_len = ret_len + strlen(buf);
        ret_buf = MEM_realloc(ret_buf, new_len + 1);
        if (ret_len == 0) {
            strcpy(ret_buf, buf);
        } else {
            strcat(ret_buf, buf);
        }
        ret_len = new_len;
        if (ret_buf[ret_len-1] == '\n') {
            break;
        }
    }
    if (ret_len > 0) {
        value.type = MRSK_STRING_VALUE;
        value.u.object = MRSK_create_murasaki_string(interpreter, env, ret_buf);
    } else {
        value.type = MRSK_NONE_VALUE;
    }

    return value;
}

MRSK_Value mrsk_nv_fputs_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                              int arg_count, MRSK_Value *args)
{
    MRSK_Value value;
    FILE *fp;

    check_argument_count(arg_count, 2);
    value.type = MRSK_NONE_VALUE;
    if (args[0].type != MRSK_STRING_VALUE
        || (args[1].type != MRSK_NATIVE_POINTER_VALUE
        || !check_native_pointer(&args[1]))) {
        mrsk_runtime_error(0, FPUTS_ARGUMENT_TYPE_ERR, MESSAGE_ARGUMENT_END);
    }
    fp = args[1].u.native_pointer.pointer;
    fputs(args[0].u.object->u.string.string, fp);

    return value;
}

void mrsk_add_std_fp(MRSK_Interpreter *inter)
{
    MRSK_Value fp_value;

    fp_value.type = MRSK_NATIVE_POINTER_VALUE;
    fp_value.u.native_pointer.info = &st_native_lib_info;

    fp_value.u.native_pointer.pointer = stdin;
    MRSK_add_global_variable(inter, "STDIN", &fp_value);

    fp_value.u.native_pointer.pointer = stdout;
    MRSK_add_global_variable(inter, "STDOUT", &fp_value);

    fp_value.u.native_pointer.pointer = stderr;
    MRSK_add_global_variable(inter, "STDERR", &fp_value);

}

MRSK_Value new_array_sub(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                         int arg_count, MRSK_Value *args, int arg_idx)
{
    MRSK_Value ret;
    int size;
    int i;

    if (args[arg_idx].type != MRSK_INT_VALUE) {
        mrsk_runtime_error(0, NEW_ARRAY_ARGUMENT_TYPE_ERR, MESSAGE_ARGUMENT_END);
    }

    size = args[arg_idx].u.int_value;

    ret.type = MRSK_ARRAY_VALUE;
    ret.u.object = MRSK_create_array(inter, env, size);

    if (arg_idx == arg_count-1) {
        for (i=0; i<size; i++) {
            ret.u.object->u.array.array[i].type = MRSK_NONE_VALUE;
        }
    } else {
        for (i=0; i<size; i++) {
            ret.u.object->u.array.array[i] = new_array_sub(inter, env, arg_count, args, arg_idx+1);
        }
    }

    return ret;
}

MRSK_Value mrsk_nv_new_array_proc(MRSK_Interpreter *interpreter, MRSK_LocalEnvironment *env,
                                  int arg_count, MRSK_Value *args)
{
    MRSK_Value value;

    if (arg_count < 1) {
        mrsk_runtime_error(0, ARGUMENT_TOO_FEW_ERR, MESSAGE_ARGUMENT_END);
    }

    value = new_array_sub(interpreter, env, arg_count, args, 0);

    return value;
}
