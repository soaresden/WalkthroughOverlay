//ft_stub.hpp

#pragma once
#ifndef FT_STUB_HPP
#define FT_STUB_HPP

// Définitions de base pour les types FreeType
#include <stdint.h>

typedef uint32_t FT_UInt32;
typedef int32_t FT_Int32;
typedef uint16_t FT_UInt16;
typedef int16_t FT_Int16;
typedef unsigned char FT_Byte;
typedef signed long FT_Fixed;
typedef int FT_Error;
typedef void* FT_Pointer;

// Constantes d'erreur
#define FT_Err_Unknown_File_Format 1

// Types pour les structures
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
typedef void* FTC_FaceID;
typedef void* FTC_Manager;
typedef void* FTC_SBitCache;
typedef void* FTC_CMapCache;

// Fonctions stub
#define FT_Init_FreeType(x) (0)
#define FT_Done_FreeType(x) 
#define FTC_Manager_New(a,b,c,d,e,f,g) (0)
#define FTC_Manager_Done(x)
#define FTC_SBitCache_New(a,b) (0)
#define FTC_CMapCache_New(a,b) (0)
#define FT_New_Face(a,b,c,d) (0)

#endif // FT_STUB_HPP