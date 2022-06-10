#!/bin/bash

make && \
echo && \
valgrind --leak-check=yes ./bin/mylisp test
