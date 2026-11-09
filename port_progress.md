# Shrek Kart — Progreso del port (iOS → PS Vita)

## 2026-11-04 — Inicialización y runtime completo (todo offline; falta solo probar en consola)

### Hecho
- `psvita-toolkit init --ipa "IPA/Shrek Kart_v.1.1.0.ipa" --name "Shrek Kart" --titleid PSVSK0001 --slug shrekkart --vita-ip 192.168.3.15` (commit del scaffold).
- Binario `ShrekKarting`: armv6 (ARM), cryptid 0 (descifrado). MinimumOSVersion 2.2.1.
- Motor: Gameloft C++ propio (karts 3D). Usa `GL_OES_matrix_palette` (skinning por GPU) → emulado en CPU en `gles.c`.
- Arranque: Sin nib (`UIApplicationMain` con delegate por nombre).
- **El boilerplate iOS que copia el toolkit no compila ni tiene runtime ObjC** → se reemplazó por el loader probado de Inotia2-vita + un runtime nuevo y común a los 6 ports iOS (copia maestra en `Ferrari-GT-Evolution-vita/source`, se sincroniza con `tools/sync_runtime.sh <port>`). Detalle en `PORTING_PLAN.md` §1.
- `tools/ios_prep.py` → `rebase.bin` **22169** palabras, `binds.bin` **855** slots / 379 símbolos. Modo: clásico sin relocs (heurístico + estructural). Cobertura de `__text`: 94.6%; 14184 literales puntero, 697 palabras de tablas de salto; 6061 candidatos rechazados (líneas `DUDA` de `report.txt`).
  - El clasificador se validó contra HAWX (que trae símbolos): 0 punteros perdidos, 0 falsos positivos en datos.
- Cobertura de imports: todos resuelven (implementación, clase vacía autogenerada, NSString constante sintetizada o stub `unimpl` que lo registra en el log).
- C++: `tools/gen_cxx.py` mapea la libstdc++ de iOS (ABI COW) a la de VitaSDK (`source/cxx_imports.c`).
- Audio: OpenAL; efectos `.adp` (ADPCM de Gameloft, lo decodifica el juego). Sin AudioToolbox de archivos.
- Video: `Shrek.m4v` → se saltea.
- Controles: Volante por acelerómetro → stick izquierdo; botones (turbo/ítem) → `controls.txt`.
- Rotación del drawable: 1 (no declarada: confirmar con SELECT+START).
- Build OK: `psvita-toolkit build --preset debug` → `build/shrekkart.vpk`.
- Datos preparados: `tools/stage_data.sh` → `ux0_data/shrekkart/` (bundle, `rebase.bin`, `binds.bin`, `controls.txt`, música `.ogg`).

### Para probar en consola (Vita 192.168.3.15, VitaShell FTP en 1337)
1. Instalar `build/shrekkart.vpk`.
2. `python3 -I tools/deploy_data.py` (sube `ux0_data/shrekkart/` → `ux0:data/shrekkart/`).
3. Requisitos en la consola: `kubridge.skprx` en `ur0:tai/config.txt` y `libshacccg.suprx` (ShaRKBR33D).
4. Abrir el juego; si se cierra o queda en negro: `python3 -I tools/deploy_data.py --logs` y leer `logs/shrekkart_NNN.log` (+ `.psp2dmp`).
5. Rotación incorrecta: mantener SELECT+START 2 s (cicla y guarda `rotation.txt`).
6. Botones: editar `ux0:data/shrekkart/controls.txt` por FTP (coordenadas de lo que se ve), sin recompilar.

### Riesgos conocidos
- Skinning por paleta emulado en CPU: verificar personajes/karts y rendimiento.

### Revisión independiente del runtime (misma sesión, antes de la consola)
Un revisor aparte leyó asm.S/objc.c/macho.c. Se corrigió:
- **Crítico:** las cadenas ObjC (selectores, nombres de clase, UTF-16) viven en `__TEXT` y se leían antes de `macho_finalize()` (el bloque RX todavía estaba vacío). Ahora `foundation_fix_cfstrings` + `objc_init_image` corren después; las escrituras en `__TEXT` van por `macho_write_word` (kubridge).
- Pool de autorelease por hilo (TLS); antes era uno solo compartido → use-after-free con hilos/GCD.
- Caché de métodos sin carrera (cls=0 → imp/sel → cls, con barreras y re-chequeo).
- `-dealloc` no puede correr dos veces (rc centinela durante el dealloc).
- `methodForSelector:` devuelve un thunk con `ml_align_call` para IMPs nativas; `objc_ivar_set` libera el valor anterior.
- Pendiente (latente, sin uso en estos binarios): `+initialize`/`+load` no se envían; `nil_imp` en llamadas `_stret` deja el struct sin tocar.
- Verificado correcto por el revisor: balance de pila/registros de todos los trampolines, `ml_dispatch_imp` (Thumb incluido), `objc_msgSendSuper2`, setjmp, cadena de metaclases, mapeo y thunks de macho.c, firmas nativas con CGRect/NSRange/double.

## 2026-11-09 — Corrida 001: pantalla negra (0 draw calls)
- Log: `fopen(ux0:/data/shrekkart/ShrekKarting.app/shaderSettings.bar) -> 0x0` → el juego hace chdir al padre del
  bundle y arma la ruta con `getcwd` (llega con `ux0:`), y `darwin_path` dejaba pasar tal cual toda ruta con `:`.
- Fix (ya estaba en la copia maestra): mapear `.app/` → `game_app/` antes del chequeo de `:`.
  Se resincronizó todo el runtime desde Ferrari (`tools/sync_runtime.sh`), que trae además los arreglos de
  imagen/sonido/toques de ese port. Build OK; falta probar en consola.

## 2026-11-09 — Corrida 002: se cierra solo (`operator new(2550146352): sin memoria`)
- 2550146352 = 0x98002530: el literal `sizeof(inflate_state)` = 0x2530 de `_inflateInit2_` (palabra en 0x6C598,
  `zalloc(opaque, 1, 0x2530)`) coincide con el inicio de una función (static init en 0x2530) y el rebase
  heurístico lo tomó por puntero. Lo usa `LZMAFile::Open` → `shaderSettings.bar` y todo recurso comprimido.
- Fix: `patch.c` restaura las palabras de `not_pointers[]` (antes de `macho_finalize`).
- Runtime común: `operator new` fallido ahora loguea la dirección del Mach-O que lo llamó.

## 2026-11-09 — Corrida 003: cuelgue (watchdog) tras abrir shaderSettings.bar
- Dump: hilo del juego girando en `_inflate` (0x68194) llamado desde `LZMAFile::readInt` (0x2065C) ←
  `ShaderMgr::LoadShaders`; en la pila `0xFFFFFFFD` = `Z_DATA_ERROR` (readInt reintenta para siempre).
- Causa: `rebase.bin` se había generado con el `ios_prep.py` viejo; rebaseaba 17 palabras de las tablas
  `short` de zlib en `__TEXT,__const` (lext/dext/order: 0x0004_0003 cae en el rango de la imagen).
- Fix: regenerar con el `ios_prep.py` sincronizado (regla `numeric_table`): 22169 → 22032 palabras
  (137 falsos punteros menos, 0 nuevos). `0x6C598` sigue rebaseado → queda el parche de `patch.c`.
- Al sincronizar el runtime hay que **regenerar siempre rebase.bin/binds.bin** (`tools/stage_data.sh`).

## 2026-11-09 — Corrida 005: carga todos los .bar y sube texturas; crash 0x30002 en CMatrix::RotateZ
- Dump: Undefined Instruction en 0x34B3C (`vmul.f32 s8,s16,s0`) tras `vmsr fpscr` con LEN=3: VFPv2 short
  vector (ARMv6), que el Cortex-A9 no implementa. 18 `vmsr fpscr` = 9 bloques en 9 funciones de `CMatrix`
  (Mult, PreMult, SetMult, Scale, SetQuaternionAndScale, Translate, RotateX/Y/Z), los 9 idénticos.
- Fix (runtime común, `macho.c: patch_vfp_vectors`, antes de copiar __TEXT a RX): vmsr de entrada → nop y
  las 8 operaciones → NEON por escalar (s8/s12 → q2/q3, s16..s28 → q4..q7, s0..s7 → d0[0]..d3[1]).
  Bloques con una operación VFP fuera de la tabla no se tocan y se loguean. Verificado en host: 9 bloques.
- Pendiente de mirar: `GLERR glPixelStorei(GL_UNPACK_ALIGNMENT)` / `glActiveTexture` → 0x500 (no crashean).

## 2026-11-09 — Corrida 006: ¡in-game! Rendimiento 8–17 fps en carrera
- PERF en carrera: total 60–125 ms/frame; **skin 47–108 ms** (150–190 dibujos con paleta), juego ~11 ms,
  draw ~3 ms. El cuello es la emulación en CPU de `GL_OES_matrix_palette`.
- Causas: `skin()` transformaba los vértices `[0, maxi]` en cada dibujo (Gameloft dibuja cada trozo del
  modelo con pocos índices sobre un buffer compartido → el modelo entero, ~170 veces por frame) y
  `read_comp` hacía un `switch` de tipo por componente.
- Fix (runtime común `gles.c`): `skin(lo, hi)` con el rango real `[mini, maxi]`; cada atributo se pasa a
  floats una vez por dibujo con un bucle por tipo (`attr_floats`, probado en host); normales solo si hay.
  PERF ahora muestra `skin ms(dibujos, vértices)`.

## 2026-11-09 — Corrida 007: skin 3–5 ms (antes 47–108), carrera 25 fps estables (= tope del juego)
- `ThreadRoutine` (0x6EA4C) limita a 1000/25 ms por frame (18 fps en algunos estados): 25 fps es el diseño.
- Audio feo: el hilo mezclador de openal-soft (backend vita.c) hereda prioridad del juego − 1 y no tiene
  núcleo fijo → compite con el juego. Fix (runtime común `openal.c`): definir los símbolos débiles que lee el
  backend, `_oal_thread_priority = 64` y `_oal_thread_affinity = SCE_KERNEL_CPU_MASK_USER_2` (como Asphalt 5:
  mezclador en su propio núcleo, prioridad alta). El streaming de música (StreamingSound: 5 buffers × ~266 ms,
  44.1 kHz estéreo) no se queda sin datos a 25 fps; `alBufferDataStatic` por copia es correcto (el juego pasa
  los datos en cada llamada).
- Logo de Gameloft negro salvo el sprite: `GS_GLLogo::Render` estado 0 solo pinta el sprite y depende del
  color de borrado del juego; nuestro present ponía `glClearColor(0,0,0,1)` cada frame y no lo restauraba.
  Fix (`gles.c`): se guarda el color del juego (`glClearColor`/`glClearColorx`) y se restaura tras el present.
- Caídas en menú: coinciden con cargas de .bar y con capturas TGA de diagnóstico → `PORT_GL_DIAG 0`.

## Corrida 008 — música "suena horrible"
- Log: 25 fps estables en carrera/menú (el fix de mezclador núcleo 2/prio 64 mejoró fluidez), audio sigue mal.
- Causa: `ios_prep.py` rebaseaba 55 palabras de `__TEXT,__const` que apuntaban "a __DATA": son las tablas
  Huffman/cuantización del decodificador Musepack (`_mpc_table_HuffQ1/Q2/Q7`, `_Di_opt`, `___Cc`; p. ej.
  0x143724 = 0x1a0006 = {code,len} empaquetado). Con +0x98000000 la música MPC sale como ruido.
- Fix: en `__TEXT,__const` solo se rebasean punteros a inicios de cstring (`data-textconst->data` en report.txt).
  rebase.bin 22032 -> 21977 palabras. Copiado a Ferrari (maestro de tools).
- Subir solo `ux0:data/shrekkart/rebase.bin` (el VPK no cambia).
