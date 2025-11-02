/* config.h.  Generated manually for RT-Thread porting.  */
/* This config header is adapted from the original autoconf-generated version. */

/* Debug interfaces enabled */
/* #undef EC_DEBUG_IF */

/* Debug ring enabled */
/* #undef EC_DEBUG_RING */

/* EoE support enabled */
//#define EC_EOE 1

/* Use CPU timestamp counter */
/* #undef EC_HAVE_CYCLES */

/* Use vendor id / product code wildcards */
/* #undef EC_IDENT_WILDCARDS */

/* Max. number of Ethernet devices per master */
#define EC_MAX_NUM_DEVICES 1

/* Read alias adresses from register */
/* #undef EC_REGALIAS */

/* RTDM interface enabled */
/* #undef EC_RTDM */

/* Use Xenomai3 RTDM flavour */
/* #undef EC_RTDM_XENOMAI_V3 */

/* Output to syslog in RT context */
#define EC_RT_SYSLOG 1

/* Assign SII to PDI */
#define EC_SII_ASSIGN 1

/* Use hrtimer for scheduling */
/* #undef EC_USE_HRTIMER */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "ethercat"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "fp@igh.de"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ethercat"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "ethercat 1.6.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "ethercat"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.6.1"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.6.1"
