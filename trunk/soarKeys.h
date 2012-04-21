#ifndef SOARKEYS_H
#define SOARKEYS_H


//PRIVATE_KEY
// This is the actual key for the encryption
R_RSA_PRIVATE_KEY PRIVATE_KEY1 = {
// bits
512,
// modulus
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xe0, 0x78, 0xc8, 0xf9, 0x2a, 0xc3, 0xd7, 0xb1, 0x6a, 0xeb, 0x66, 0x00, 0x15, 0xed, 0x54, 0x28, 
0x91, 0x81, 0x35, 0x9f, 0x6f, 0x8a, 0xb3, 0x6a, 0xac, 0x9d, 0x0a, 0x75, 0x12, 0xdf, 0x9e, 0x77, 
0xf0, 0xd3, 0x3c, 0x30, 0xf4, 0xad, 0x33, 0x6a, 0x9a, 0x65, 0x24, 0x20, 0x89, 0x8b, 0xf7, 0x95, 
0x9d, 0xbe, 0x9a, 0xbd, 0x31, 0x77, 0x80, 0xac, 0x14, 0x0f, 0xa4, 0x90, 0xe6, 0x0d, 0x0a, 0x29
},
// publicExponent
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01
},
// exponent
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x93, 0x67, 0x3d, 0xb2, 0x41, 0x9e, 0xf1, 0x59, 0x14, 0x39, 0x18, 0x76, 0x1d, 0xf0, 0x07, 0x3f, 
0xcc, 0xac, 0xe8, 0xa5, 0x95, 0xfd, 0xa2, 0xeb, 0xfe, 0x05, 0xf2, 0x04, 0x07, 0x2c, 0xc9, 0x46, 
0x06, 0x6b, 0xf5, 0x47, 0x6e, 0xdc, 0xf4, 0x15, 0xf4, 0x27, 0x05, 0xb7, 0x79, 0x15, 0xe2, 0xdc, 
0xb3, 0x9e, 0x29, 0x25, 0xbd, 0xaa, 0x33, 0x1c, 0x67, 0xd4, 0x71, 0x1b, 0xf6, 0x8c, 0x85, 0x31
},
{
// prime0
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xff, 0x5e, 0x6e, 0xe1, 0xc3, 0xea, 0x31, 0x41, 0x90, 0xe0, 0x8e, 0x24, 0x47, 0x5e, 0xb8, 0x16, 
0xf1, 0x21, 0x1e, 0xd1, 0x49, 0xff, 0x95, 0x33, 0xe5, 0x60, 0xde, 0x4c, 0xae, 0xae, 0xf8, 0x37
},
// prime1
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xe1, 0x06, 0xcd, 0xd0, 0x08, 0xc9, 0x15, 0x09, 0x14, 0x93, 0x8a, 0x8c, 0xa5, 0xd7, 0x84, 0xb8, 
0x56, 0x15, 0x42, 0x38, 0xb7, 0xe7, 0x8e, 0x89, 0xcb, 0xd6, 0x56, 0x9f, 0x59, 0x76, 0x20, 0x9f
}
},
{
// primeExponent0
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x40, 0x0b, 0x3a, 0xdb, 0xbb, 0xfa, 0x9b, 0xe7, 0xc9, 0xa1, 0xc7, 0x84, 0x29, 0xb2, 0x03, 0x91, 
0x1d, 0x60, 0x25, 0x0e, 0x6f, 0xf4, 0x7d, 0x42, 0xca, 0xa0, 0x04, 0xa0, 0x4e, 0x9e, 0xea, 0xf9
},
// primeExponent1
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x71, 0x76, 0x71, 0xce, 0x44, 0xaf, 0xc7, 0x68, 0x85, 0xb4, 0x83, 0x36, 0xb9, 0xe4, 0x7a, 0xaa, 
0x4b, 0xd5, 0x7a, 0x47, 0x89, 0x0b, 0x3b, 0xc6, 0xe5, 0x3d, 0xd4, 0xfd, 0x92, 0x29, 0x4a, 0xf7
}
},
// coefficient
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xcd, 0xde, 0x32, 0x88, 0x1d, 0x0a, 0x09, 0x49, 0x18, 0x17, 0x1c, 0xcb, 0xe5, 0xfc, 0x70, 0x43, 
0x33, 0xf0, 0x00, 0x1b, 0x59, 0xf2, 0x85, 0x90, 0x44, 0x2d, 0x01, 0xc9, 0x64, 0xe3, 0x38, 0xcf
}
};
#endif /* SOARRSA_H */
