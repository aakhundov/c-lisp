#include "eval.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

typedef value* (*op_fn)(value**, size_t);

static value* op_add(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result += (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_subtract(value** args, size_t num_args) {
    if (num_args == 1) {
        return value_new_number(-(*args)->number);
    }

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result -= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_multiply(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result *= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_divide(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        if ((*args)->number == 0) {
            return value_new_error("division by zero");
        }
        result /= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_modulo(value** args, size_t num_args) {
    int result = (int)(*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result %= (int)(*args++)->number;
    }

    return value_new_number(result);
}

static value* op_power(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result = pow(result, (*args++)->number);
    }

    return value_new_number(result);
}

static value* op_minimum(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
        if (other < result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static value* op_maximum(value** args, size_t num_args) {
    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
        if (other > result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static op_fn get_op_fn(char* op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        return op_add;
    } else if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        return op_subtract;
    } else if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        return op_multiply;
    } else if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
        return op_divide;
    } else if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
        return op_modulo;
    } else if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) {
        return op_power;
    } else if (strcmp(op, "min") == 0) {
        return op_minimum;
    } else if (strcmp(op, "max") == 0) {
        return op_maximum;
    } else {
        return NULL;
    }
}

value* value_evaluate(value* v) {
    if (v->type == VALUE_SEXPR) {
        value* result = NULL;
        value* temp = value_new_sexpr();

        for (size_t i = 0; i < v->num_children; i++) {
            value* evaled = value_evaluate(v->children[i]);

            if (evaled->type == VALUE_ERROR) {
                result = evaled;
                break;
            } else {
                value_add_child(temp, evaled);
            }
        }

        if (result == NULL) {
            if (temp->num_children == 0) {
                return temp;
            } else if (temp->num_children == 1) {
                result = temp->children[0];
                temp->children[0] = NULL;
            }
        }

        if (result == NULL) {
            char buffer[1024];
            value* op_value = temp->children[0];

            if (op_value->type != VALUE_SYMBOL) {
                value_to_str(v, buffer);
                result = value_new_error("s-expr doesn't start with a symbol: '%s'", buffer);
            } else {
                op_fn op = get_op_fn(op_value->symbol);

                if (op == NULL) {
                    result = value_new_error("unrecognizer operator: '%s'", op_value->symbol);
                } else {
                    for (size_t i = 1; i < temp->num_children; i++) {
                        if (temp->children[i]->type != VALUE_NUMBER) {
                            value_to_str(temp->children[i], buffer);
                            result = value_new_error("non-numeric argument: '%s'", buffer);
                            break;
                        }
                    }

                    if (result == NULL) {
                        result = op(temp->children + 1, temp->num_children - 1);
                    }
                }
            }
        }

        value_dispose(temp);

        return result;
    } else {
        return value_copy(v);
    }
}
