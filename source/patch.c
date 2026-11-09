// Parches de este juego. Vacío hasta la primera corrida en consola: cada parche se agrega con
// evidencia (log/dump + desensamblado con tools/annotate.py) y su dirección del Mach-O.
#include "imports.h"
#include "log.h"
#include "macho.h"
#include "patch.h"

const import_entry patch_imports[] = {
    {"", 0, 0},
};
TABLE_END(patch)

// Literales de __text que el rebase heurístico tomó por punteros (coinciden con un inicio de función)
// y en realidad son enteros: se restauran al valor original.
static const uint32_t not_pointers[] = {
    0x6C598, // _inflateInit2_: sizeof(inflate_state) = 0x2530 -> zalloc(opaque, 1, 0x2530); rebaseado daba
             // new[](0x98002530) "sin memoria" (corrida 002)
};

void patch_init(void) {
    for (unsigned i = 0; i < sizeof(not_pointers) / sizeof(not_pointers[0]); i++) {
        uint32_t v = macho_read_text(not_pointers[i]);
        if (v >= MACHO_SLIDE)
            v -= MACHO_SLIDE;
        macho_patch_text(not_pointers[i], &v, 4);
    }
}
