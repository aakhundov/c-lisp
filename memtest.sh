make
echo
valgrind --leak-check=yes ./bin/clisp test 2> >(grep -i 'main')
