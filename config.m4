PHP_ARG_ENABLE(geometry, 
	whether to enable Geometry support, 
	[ --enable-geometry   Enable Geometry support])
if test "$PHP_GEOMETRY" = "yes"; then
  AC_DEFINE(HAVE_GEOMETRY, 1, [Whether you have Geometry])
  PHP_NEW_EXTENSION(geometry, geometry.c coordinate.c coordinate_set.c distance_map.c helmert.c task.c statistics/*.c formatter/formatter_*.c string_manip.c include/json/*.c, $ext_shared)
fi
