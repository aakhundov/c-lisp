#include "value.h"

#include <stdio.h>
#include <string.h>

value value_new_number(double number) {
    value v;

    v.type = VALUE_NUMBER;
    v.number = number;

    return v;
}

value value_new_error(error_type error, char* error_arg) {
    value v;

    v.type = VALUE_ERROR;
    v.error = error;

    if (error_arg != 0) {
        strncpy(v.error_arg, error_arg, sizeof(v.error_arg) - 1);
        v.error_arg[sizeof(v.error_arg) - 1] = '\0';
    } else {
        v.error_arg[0] = '\0';
    }

    return v;
}

void value_to_str(value* v, char* buffer) {
    switch (v->type) {
        case VALUE_NUMBER:
            sprintf(buffer, "%g", v->number);
            break;
        case VALUE_ERROR:
            switch (v->error) {
                case ERROR_DIV_ZERO:
                    sprintf(buffer, "error: division by zero");
                    break;
                case ERROR_BAD_NUMBER:
                    sprintf(buffer, "error: malformed number \"%s\"", v->error_arg);
                    break;
                case ERROR_BAD_OP:
                    sprintf(buffer, "error: unrecognizer operator \"%s\"", v->error_arg);
                    break;
                default:
                    sprintf(buffer, "unknown error type: %d", v->error);
            }
            break;
        default:
            sprintf(buffer, "unknown value type: %d", v->type);
    }
}
