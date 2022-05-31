#ifndef VALUE_H_
#define VALUE_H_

typedef enum {
    VALUE_NUMBER = 0,
    VALUE_ERROR = 1
} value_type;

typedef enum {
    ERROR_DIV_ZERO = 0,
    ERROR_BAD_NUMBER = 1,
    ERROR_BAD_OP = 2
} error_type;

typedef struct {
    value_type type;
    double number;
    error_type error;
    char error_arg[32];
} value;

value value_new_number(double number);
value value_new_error(error_type error, char* error_arg);

void value_to_str(value* v, char* buffer);

#endif  // VALUE_H_
