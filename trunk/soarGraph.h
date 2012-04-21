#ifndef SOARGRAPH_H
#define SOARGRAPH_H

#define SGRA __attribute__ ((section ("sgra")))

void DrawAltTimeGraph(Int16 fltindex, UInt8 graphtype) SGRA;
void DrawPctThermalGraph(Int16 fltindex, UInt8 graphtype) SGRA;

#endif
