/*                                                                -*- C -*-
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig Sæther Bakken <ssb@php.net>                             |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#define CONFIGURE_COMMAND " './configure'  '--build=x86_64-redhat-linux-gnu' '--host=x86_64-redhat-linux-gnu' '--program-prefix=' '--disable-dependency-tracking' '--prefix=/usr' '--exec-prefix=/usr' '--bindir=/usr/bin' '--sbindir=/usr/sbin' '--sysconfdir=/etc' '--datadir=/usr/share' '--includedir=/usr/include' '--libdir=/usr/lib64' '--libexecdir=/usr/libexec' '--localstatedir=/var' '--sharedstatedir=/var/lib' '--mandir=/usr/share/man' '--infodir=/usr/share/info' '--cache-file=../config.cache' '--with-libdir=lib64' '--with-config-file-path=/etc' '--with-config-file-scan-dir=/etc/php.d' '--disable-debug' '--with-pic' '--disable-rpath' '--without-pear' '--with-exec-dir=/usr/bin' '--with-freetype-dir=/usr' '--with-png-dir=/usr' '--with-xpm-dir=/usr' '--with-vpx-dir=/usr' '--enable-gd-native-ttf' '--with-t1lib=/usr' '--without-gdbm' '--with-jpeg-dir=/usr' '--with-openssl' '--with-system-ciphers' '--with-pcre-regex=/usr' '--with-zlib' '--with-layout=GNU' '--with-kerberos' '--with-libxml-dir=/usr' '--with-system-tzdata' '--with-mhash' '--enable-dtrace' '--libdir=/usr/lib64/php' '--enable-pcntl' '--enable-opcache' '--enable-phpdbg' '--with-imap=shared' '--with-imap-ssl' '--enable-mbstring=shared' '--enable-mbregex' '--with-gd=shared,/usr' '--with-gmp=shared' '--enable-calendar=shared' '--enable-bcmath=shared' '--with-bz2=shared' '--enable-ctype=shared' '--enable-dba=shared' '--with-db4=/usr' '--with-gdbm=/usr' '--with-tcadb=/usr' '--enable-exif=shared' '--enable-ftp=shared' '--with-gettext=shared' '--with-iconv=shared' '--enable-sockets=shared' '--enable-tokenizer=shared' '--with-xmlrpc=shared' '--with-ldap=shared' '--with-ldap-sasl' '--enable-mysqlnd=shared' '--with-mysql=shared,mysqlnd' '--with-mysqli=shared,mysqlnd' '--with-mysql-sock=/var/lib/mysql/mysql.sock' '--with-oci8=shared,instantclient,/usr/lib64/oracle/12.1/client64/lib,12.1' '--with-pdo-oci=shared,instantclient,/usr,12.1' '--with-interbase=shared,/usr/lib64/firebird' '--with-pdo-firebird=shared,/usr/lib64/firebird' '--enable-dom=shared' '--with-pgsql=shared' '--enable-simplexml=shared' '--enable-xml=shared' '--enable-wddx=shared' '--with-snmp=shared,/usr' '--enable-soap=shared' '--with-xsl=shared,/usr' '--enable-xmlreader=shared' '--enable-xmlwriter=shared' '--with-curl=shared,/usr' '--enable-pdo=shared' '--with-pdo-odbc=shared,unixODBC,/usr' '--with-pdo-mysql=shared,mysqlnd' '--with-pdo-pgsql=shared,/usr' '--with-pdo-sqlite=shared,/usr' '--with-pdo-dblib=shared,/usr' '--with-sqlite3=shared,/usr' '--without-readline' '--with-libedit' '--with-pspell=shared' '--enable-phar=shared' '--with-mcrypt=shared,/usr' '--with-tidy=shared,/usr' '--with-mssql=shared,/usr' '--enable-sysvmsg=shared' '--enable-sysvshm=shared' '--enable-sysvsem=shared' '--enable-shmop=shared' '--enable-posix=shared' '--with-unixODBC=shared,/usr' '--enable-fileinfo=shared' '--enable-intl=shared' '--with-icu-dir=/usr' '--with-enchant=shared,/usr' '--with-recode=shared,/usr' 'build_alias=x86_64-redhat-linux-gnu' 'host_alias=x86_64-redhat-linux-gnu' 'CFLAGS=-O2 '-g' '-pipe' '-Wall' '-Wp,-D_FORTIFY_SOURCE=2' '-fexceptions' '-fstack-protector-strong' '--param=ssp-buffer-size=4' '-grecord-gcc-switches' '-specs=/usr/lib/rpm/redhat/redhat-hardened-cc1' '-m64' '-mtune=generic' '-fno-strict-aliasing' '-Wno-pointer-sign'' 'LDFLAGS=-Wl,-z,relro '-specs=/usr/lib/rpm/redhat/redhat-hardened-ld'' 'CXXFLAGS=-O2 '-g' '-pipe' '-Wall' '-Wp,-D_FORTIFY_SOURCE=2' '-fexceptions' '-fstack-protector-strong' '--param=ssp-buffer-size=4' '-grecord-gcc-switches' '-specs=/usr/lib/rpm/redhat/redhat-hardened-cc1' '-m64' '-mtune=generic''"
#define PHP_ADA_INCLUDE		""
#define PHP_ADA_LFLAGS		""
#define PHP_ADA_LIBS		""
#define PHP_APACHE_INCLUDE	""
#define PHP_APACHE_TARGET	""
#define PHP_FHTTPD_INCLUDE      ""
#define PHP_FHTTPD_LIB          ""
#define PHP_FHTTPD_TARGET       ""
#define PHP_CFLAGS		"$(CFLAGS_CLEAN) -prefer-non-pic -static"
#define PHP_DBASE_LIB		""
#define PHP_BUILD_DEBUG		""
#define PHP_GDBM_INCLUDE	""
#define PHP_IBASE_INCLUDE	""
#define PHP_IBASE_LFLAGS	""
#define PHP_IBASE_LIBS		""
#define PHP_IFX_INCLUDE		""
#define PHP_IFX_LFLAGS		""
#define PHP_IFX_LIBS		""
#define PHP_INSTALL_IT		""
#define PHP_IODBC_INCLUDE	""
#define PHP_IODBC_LFLAGS	""
#define PHP_IODBC_LIBS		""
#define PHP_MSQL_INCLUDE	""
#define PHP_MSQL_LFLAGS		""
#define PHP_MSQL_LIBS		""
#define PHP_MYSQL_INCLUDE	""
#define PHP_MYSQL_LIBS		""
#define PHP_MYSQL_TYPE		""
#define PHP_ODBC_INCLUDE	"-I/usr/include"
#define PHP_ODBC_LFLAGS		"-L/usr/lib64"
#define PHP_ODBC_LIBS		"-lodbc"
#define PHP_ODBC_TYPE		"unixODBC"
#define PHP_OCI8_SHARED_LIBADD 	"-L/usr/lib64/oracle/12.1/client64/lib  -lclntsh"
#define PHP_OCI8_DIR			"/usr/lib64/oracle/12.1/client64/lib"
#define PHP_OCI8_ORACLE_VERSION		"12.1"
#define PHP_ORACLE_SHARED_LIBADD 	"@ORACLE_SHARED_LIBADD@"
#define PHP_ORACLE_DIR				"@ORACLE_DIR@"
#define PHP_ORACLE_VERSION			"@ORACLE_VERSION@"
#define PHP_PGSQL_INCLUDE	""
#define PHP_PGSQL_LFLAGS	""
#define PHP_PGSQL_LIBS		""
#define PHP_PROG_SENDMAIL	"/usr/sbin/sendmail"
#define PHP_SOLID_INCLUDE	""
#define PHP_SOLID_LIBS		""
#define PHP_EMPRESS_INCLUDE	""
#define PHP_EMPRESS_LIBS	""
#define PHP_SYBASE_INCLUDE	""
#define PHP_SYBASE_LFLAGS	""
#define PHP_SYBASE_LIBS		""
#define PHP_DBM_TYPE		""
#define PHP_DBM_LIB		""
#define PHP_LDAP_LFLAGS		""
#define PHP_LDAP_INCLUDE	""
#define PHP_LDAP_LIBS		""
#define PHP_BIRDSTEP_INCLUDE     ""
#define PHP_BIRDSTEP_LIBS        ""
#define PEAR_INSTALLDIR         "/usr/share/pear"
#define PHP_INCLUDE_PATH	".:/usr/share/pear:/usr/share/php"
#define PHP_EXTENSION_DIR       "/usr/lib64/php/modules"
#define PHP_PREFIX              "/usr"
#define PHP_BINDIR              "/usr/bin"
#define PHP_SBINDIR             "/usr/sbin"
#define PHP_MANDIR              "/usr/share/man"
#define PHP_LIBDIR              "/usr/lib64/php"
#define PHP_DATADIR             "/usr/share"
#define PHP_SYSCONFDIR          "/etc"
#define PHP_LOCALSTATEDIR       "/var"
#define PHP_CONFIG_FILE_PATH    "/etc"
#define PHP_CONFIG_FILE_SCAN_DIR    "/etc/php.d"
#define PHP_SHLIB_SUFFIX        "so"
