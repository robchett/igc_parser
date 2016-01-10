pushd test
valgrind --tool=memcheck --num-callers=30 --log-file=php.log ../bin/php test.php
chmod 777 php.log
popd

# --leak-check=full --track-origins=yes -v