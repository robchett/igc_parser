PHP_ARG_ENABLE(geometry, whether to enable Hello World support, [ --enable-geometry   Enable Hello World support])
if test "$PHP_GEOMETRY" = "yes"; then
  AC_DEFINE(HAVE_GEOMETRY, 1, [Whether you have Hello World])
  PHP_NEW_EXTENSION(geometry, geometry.c coordinate.c coordinate_set.c distance_map.c helmert.c task.c formatter/formatter_*.c string_manip.c include/json/*.c, $ext_shared)
fi