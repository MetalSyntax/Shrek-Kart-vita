// Generado por tools/gen_cxx.py: imports C++ del juego -> libstdc++ de VitaSDK.
// No editar a mano (volver a correr gen_cxx.py con el report.txt de ios_prep.py).
#include <string.h>

#include "imports.h"

extern char cxx_0 __asm__("_ZNSt15_List_node_base4hookEPS_"); // std::_List_node_base::hook(std::_List_node_base*)
extern char cxx_1 __asm__("_ZNSt15_List_node_base6unhookEv"); // std::_List_node_base::unhook()
extern char cxx_2 __asm__("_ZNSt8ios_base4InitC1Ev"); // std::ios_base::Init::Init()
extern char cxx_3 __asm__("_ZNSt8ios_base4InitD1Ev"); // std::ios_base::Init::~Init()
extern char cxx_4 __asm__("_ZdaPv"); // operator delete[](void*)
extern char cxx_5 __asm__("_Znaj"); // operator new[](unsigned long)

const import_entry cxx_imports[] = {
    {"__ZNSt15_List_node_base4hookEPS_", (uintptr_t)&cxx_0, 0},
    {"__ZNSt15_List_node_base6unhookEv", (uintptr_t)&cxx_1, 0},
    {"__ZNSt8ios_base4InitC1Ev", (uintptr_t)&cxx_2, 0},
    {"__ZNSt8ios_base4InitD1Ev", (uintptr_t)&cxx_3, 0},
    {"__ZdaPv", (uintptr_t)&cxx_4, 0},
    {"__Znam", (uintptr_t)&cxx_5, 0},
};
TABLE_END(cxx)

// Lo que no tiene equivalente queda en el log como "unimpl".
const import_entry *cxx_import(const char *name) { (void)name; return NULL; }
