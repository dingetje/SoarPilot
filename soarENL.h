#ifndef SOARENL_H
#define SOARENL_H

#define SENL __attribute__ ((section ("senl")))

// Sound functions for engine noise level
Boolean ENLCapableDevice() SENL;
Boolean initENL(Boolean reset) SENL;
Int16 getENL() SENL;

#endif
