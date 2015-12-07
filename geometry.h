#ifndef PHP_GEOMETRY_H
	#define PHP_GEOMETRY_H 1
	#define PHP_GEOMETRY_VERSION "0.0.2"
	#define PHP_GEOMETRY_EXTNAME "geometry"

	#define ZEND_DEBUG 0
	//#define DEBUG_LEVEL 0

	PHP_MINIT_FUNCTION(geometry);

	char *get_os_grid_ref(coordinate_object *point);
	char *gridref_number_to_letter(long e, long n);

	#define is_object_of_type(zval_ptr, type) (zval_ptr && Z_TYPE_P(zval_ptr) == IS_OBJECT && Z_OBJCE(*zval_ptr) == type)



	#ifdef DEBUG_LEVEL
		#define note(S, ...) fprintf(stderr,                                     \
		  "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[90mnote:\x1b[0m " S "\n",    \
		  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

		#define warn(S, ...) fprintf(stderr,                                     \
		  "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[33mwarning:\x1b[0m " S "\n", \
		  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

		#define errn(S, ...) do { fprintf(stderr,                                \
		  "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[31merror:\x1b[0m " S "\n",   \
		  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); exit(1); } while (0) 
	#else 
		#define note(S, ...) ;
		#define warn(S, ...) ;
		#define errn(S, ...) ;
	#endif
#endif