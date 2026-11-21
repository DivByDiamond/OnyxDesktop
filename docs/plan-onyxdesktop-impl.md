# OnyxDesktop — план реализации

> **Для агентов:** REQUIRED SUB-SKILL: subagent-driven-development
> (рекомендуется) или executing-plans. Шаги — чекбоксы (`- [ ]`).

**Goal:** `onyxdesk` — пользовательский десктоп-демон Onyx: панель,
лаунчер, полноэкранный запуск GUI-приложений с возвратом по выходу.

**Architecture:** C99-процесс (ring 1) захватывает `/dev/fb0`
(`FB_IOCTL_CLAIM_GRAPHICS`) на весь сеанс, рисует панель/лаунчер,
запускает выбранное приложение (managed-режим: приложение рисует в fb
само и claim не делает), ждёт `wait`, возвращается. Ввод v1 — только
мышь (`SYS_mouse_read`); маршрутизация клавиш/SIGSTOP-переключение —
M3 (ядро P2-A).

**Tech Stack:** C99 + либонкс (`OnyxCompiller/libonyxc`), сборка
`onyxcc`/`onyx-ld` → ONX1, host-тесты gcc, CI GitHub Actions.

**Spec:** `../OnyxOS/docs/dev/onyxdesktop-design.md` (утверждён
2026-11-21).

## Global Constraints

- ≤200 строк на файл, ≤4 файла на папку (правила OnyxApps).
- Тесты — только в `tests/native/` (host, gcc) или `foo/tests.c`;
  в файле кода inline-тестов нет.
- Никаких `printf` с неформатированными маркерами вразнобой: маркеры
  smoke (`DESK_READY`, `APP_STARTED <name> <pid>`, `APP_RETURN <code>`,
  `DESK_EXIT <code>`) печатаются ровно один раз и грепаются скриптом.
- fb-дисциплина: claim при старте, release на любом выходе (atexit/
  обработка ошибок); managed-приложения claim не делают.
- Коммиты внутри `OnyxDesktop` (и `OnyxOS` для интеграции), `feat:`/
  `build:` префиксы; явные пути.

## Review Focus

1. **Битый/пустой конфиг** → дефолт (панель 24, приложения из встроенного
   списка), не падение. Тест: T2.
2. **fb не открылся / claim занят** → выход с кодом 3 и понятным
   сообщением, без release чужого claim. Тест: T4 host-ветка (fake fb).
3. **Приложение не запустилось** (`spawn` ошибка) → возврат в лаунчер,
   маркер `APP_RETURN` только для реально запущенных. Тест: T4.
4. **Переполнение буфера имён** в конфиге/лаунчере → отказ строки, не
   truncate молча. Тест: T2/T3.
5. **Выход по сигналу** → release fb и `DESK_EXIT`. Тест: T4 (host fake).

---

### Task 1: Скелет репо, сборка, host-тесты, CI

**Files:**
- Create: `Makefile`, `README.md`, `.gitignore`
- Create: `src/main.c` (минимальный: печатает `DESK_READY`, exit 0)
- Create: `tests/native/Makefile`, `tests/native/test_harness.c`
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Produces: `make build/onyxdesk.onx` (ONXCC=…), `make -C tests/native test`,
  CI job `native-tests` + `build`.

- [x] Шаг 1: `tests/native/test_harness.c` — `CHECK(1+1==2)` + вывод
  `test_harness: N checks, 0 failures` (образец: OnyxApps
  `tests/native/Makefile`). 2026-11-21.
- [x] Шаг 2: `make -C tests/native test` → FAIL (нет Makefile).
  2026-11-21: `No rule to make target 'test'`, exit 2.
- [x] Шаг 3: Makefile по образцу OnyxApps (`ONYXCC`/`onyx-ld`, выход
  `build/onyxdesk.onx` с ONX1); `main.c` с `DESK_READY`; CI: native
  tests + build через checkout OnyxCompiller (как OnyxApps CI).
  2026-11-21.
- [x] Шаг 4: `make -C tests/native test` PASS; сборка с
  `ONYXCC=/storage/project/systems/Onyx/OnyxCompiller/onyxcc`; ONX1.
  2026-11-21: 3 checks, 0 failures; ONX1, 402162 байта.
- [x] Шаг 5: Commit `feat: onyxdesk skeleton with build and CI`.
  2026-11-21.

### Task 2: Парсер /etc/desk.conf

**Files:**
- Create: `src/config.h`, `src/config.c`, `tests/native/config/test_config.c`
  (подпапка — лимит 4 файла на папку в `tests/native/`)

**Interfaces (точные):**
```c
#define DESK_MAX_APPS 8
#define DESK_NAME_MAX 32
struct desk_config { int panel_height; int n_apps; char apps[DESK_MAX_APPS][DESK_NAME_MAX]; };
int desk_config_parse(const char *text, struct desk_config *out); /* 0 ok, -1 bad */
void desk_config_default(struct desk_config *out);
```
Формат: `# comment`, `panel_height=24`, `app=<name>`; неизвестные
ключи игнорируются; имя ≥ DESK_NAME_MAX или apps > 8 → `-1`;
пустой/битый текст → `desk_config_default` + 0.

- [x] Шаг 1: тесты: валида, комментарии, неизвестный ключ, переполнение
  имён/списка, пустой текст → default. RED. 2026-11-21: 47 проверок в
  `tests/native/config/test_config.c`; `make -C tests/native test` →
  `Нет правила для сборки цели «../../src/config.c»`, exit 2.
- [x] Шаг 2: реализация; GREEN. 2026-11-21: 47 checks, 0 failures (плюс
  harness 3/3); ASan/UBSan прогон чистый; `.onx` собирается с
  `src/config.c` в составе, ONX1, 419683 байта.
- [x] Шаг 3: Commit `feat: desk config parser`. 2026-11-21.

### Task 3: Панель и лаунчер (чистая отрисовка)

**Files:**
- Create: `src/ui.h`, `src/ui.c`, `tests/native/test_ui.c`

**Interfaces:**
```c
/* FB_IOCTL_GET_INFO = 0x4600: пять u32 в этом порядке */
struct fb_info { unsigned width, height, bpp, pitch, size; };
void ui_draw_panel(unsigned char *frame, const struct fb_info *i, int h, const char *active);
int  ui_hit_test(const struct fb_info *i, int panel_h, int n_apps, int x, int y); /* 0..n-1 | -1 */
void ui_draw_launcher(unsigned char *frame, const struct fb_info *i, const struct desk_config *c, int selected);
```
`frame` — полный кадр `width*height*(bpp/8)` байт (вызывающий предоставляет);
present в Task 4 пишет кадр по строкам через `lseek(fd, y*pitch)` + `write`
(как `otablet/device/frontend.c`; mmap не нужен). Поддержка bpp 16
(r5g6b5) и 32 (XRGB), как там же; пиксель — одна внутренняя `put_px`.

- [x] Шаг 1: тесты на маленьком буфере: панель закрашивает ровно
  `panel_h` строк; hit-test попадает в i-й пункт и мимо → -1;
  launcher рисует пункты без выхода за буфер (bpp16 и bpp32). RED.
  2026-11-21: 102 проверки в `tests/native/ui/` (`test_ui_panel.c`,
  `test_ui_launcher.c`, `test_ui_edges.c` + общий хелпер
  `test_ui_util.h` с guard-зоной за кадром); `test_ui_launcher`
  30 checks / 8 failures — противоречивые проверки `rect_is` на
  всю строку против `has_px` текста в той же строке (чинились в
  шаге 2), `test_ui_edges` не доходил из-за остановки make.
- [x] Шаг 2: реализация; GREEN. 2026-11-21: все 152 checks / 0
  failures (harness 3, config 47, panel 42, launcher 38, edges 22);
  два теста исправлены как ошибочные (см. отчёт: rect_is
  фон-строки заменён на полосу под глифами + явная проверка
  отсутствия подсветки; clamp-проверка hit-test переведена на
  высокий кадр, где строки реально в кадре); ASan/UBSan прогон
  чистый; `.onx` собирается (ui.c + font.c в составе), ONX1,
  427860 байт.
- [x] Шаг 3: Commit `feat: panel and launcher drawing primitives`.
  2026-11-21.

### Task 4: Главный цикл, fb, spawn/wait, маркеры, test-хуки

**Files:**
- Create: `src/desk.c`, `src/desk.h`, `tests/native/test_desk.c`
- Modify: `src/main.c` (вызов `desk_run`)

**Interfaces:**
```c
struct desk_io { int (*open)(const char*, int); int (*ioctl)(int, unsigned long, unsigned long);
  void *(*mmap)(...); int (*spawn)(const char*); int (*wait)(int); /* test seams */ };
int desk_run(const struct desk_config *c, const char *autostart, int once);
```
Поведение: open `/dev/fb0` → `GET_INFO` (20 байт) → mmap → `CLAIM` →
`DESK_READY` → (autostart ? запуск + `APP_STARTED name pid` → wait →
`APP_RETURN code` → once? `DESK_EXIT 0` : цикл) → при выборе мыши
(`SYS_mouse_read`: кнопка/позиция) hit-test → spawn. Ошибки: fb/claim →
stderr + 3; spawn → в лаунчер. `SIGTERM`/`SIGINT` → release + `DESK_EXIT`.
Ссылки: `otablet/device/frontend.c` (claim/mmap/формат), запуск
приложений — код `otablet/desktop` (только как образец вызовов libonyxc).

- [x] Шаг 1: host-тесты с fake `desk_io`: нет fb → 3; spawn-ошибка →
  возврат; autostart+once → ровно `APP_STARTED`+`APP_RETURN`+`DESK_EXIT`;
  release вызывается ровно один раз. RED. 2026-11-21: 56 проверок в
  `tests/native/desk/` (`fake_desk.h` — scripted fake `desk_io` + захват
  маркеров, `test_desk.c` — стартовые отказы/autostart,
  `test_desk_events.c` — мышиный цикл; дисциплина claim проверяется
  счётчиком ioctl: RELEASE ровно один и всегда последний); `make -C
  tests/native test` → «Нет правила для сборки цели
  "../../src/desk/desk.c"», exit 2.
- [x] Шаг 2: реализация; GREEN. 2026-11-21: все 208 checks / 0 failures
  (harness 3, config 47, panel 42, launcher 38, edges 22, desk 25,
  desk_events 31); ASan/UBSan прогон чистый; `src/desk/{desk.h,desk.c,
  io.c}` + переработанный `src/main.c` (--autostart/--once,
  /etc/desk.conf → дефолт при ошибке); mmap не используется — present
  по строкам lseek+write.
- [x] Шаг 3: реальная сборка .onx; ручной запуск в QEMU interactive
  (`onyxdesk --autostart osnake --once`) — маркеры в консоли.
  2026-11-21: `make ONYXCC=…/OnyxCompiller/onyxcc build/onyxdesk.onx`
  собирается (src/desk/* + новый main.c), ONX1, 464151 байт; `--ring1`
  сохранён (Makefile:27, гейт SYS_spawn). Ручной QEMU-запуск перенесён
  в таск 5: onyxdesk ещё не упакован в образ (упаковка — таск 5).
- [x] Шаг 4: Commit `feat: desk main loop with managed app launch`.
  2026-11-21.

### Task 5: Интеграция OnyxOS + smoke

**Files:**
- Modify (OnyxOS): `scripts/dependencies.py` (7-я зависимость),
  `scripts/build-apps.py` (или новый `scripts/package-desktop.py`),
  `scripts/mk-onyxfs-disk.sh` (класть `/bin/onyxdesk` + `/etc/desk.conf`)
- Create (OnyxOS): `scripts/qemu-desk-smoke.sh`
- Modify (OnyxOS): `docs/dev/roadmap.md`, `OnyxKernel/todo.md` (P2 GUI)

- [x] Шаг 1: включить `OnyxDesktop` в зависимости/образ; `desk.conf`
  с `app=osnake`, `app=ochat-gui`. 2026-11-21: `etc/desk.conf` в
  `7e131ec`; 7-я запись в `Onyx.vent`/`Onyx.lock.json`, шаг 4.5 в
  `build-all.sh`, `package-desktop.py` кладёт `/bin/onyxdesk` +
  `/etc/desk.conf` в rootfs.
- [x] Шаг 2: smoke: boot chain → логин root → `onyxdesk --autostart osnake
  --once` → grep `DESK_READY`, `APP_STARTED osnake`, `APP_RETURN`,
  `DESK_EXIT 0`; exit 0. 2026-11-21: `qemu-desk-smoke.sh` PASS (180 c
  budget, ramfb): `DESK_READY` → `APP_STARTED osnake 4` → `APP_RETURN 0`
  → `DESK_EXIT 0`, 0 «illegal instruction».
- [x] Шаг 3: прогнать `qemu-smoke.sh`/`qemu-interactive-smoke.sh` — не
  сломаны. 2026-11-21: оба PASS (exit 0) на образе с onyxdesk.
- [x] Шаг 4: docs sync; Commit `build: package and smoke-test onyxdesk`.
  2026-11-21: блок OnyxDesktop в `OnyxOS/docs/dev/roadmap.md` (v0.6),
  D1 в `OnyxKernel/todo.md` (`774eeb2`), спека
  `docs/dev/onyxdesktop-design.md` в OnyxOS.

### Финальный гейт

- [x] `make -C tests/native test` в OnyxDesktop. 2026-11-21: 237
  checks / 0 failures.
- [x] `bash OnyxOS/scripts/qemu-desk-smoke.sh`. 2026-11-21: PASS.
- [x] Качество для C: сборка `.onx` + ONX1 (Rust-only `quality/check.sh`
  неприменим). 2026-11-21: locked-пайплайн собирает
  `build/onyxdesk.onx` (454K), ONX1 v1 c ring-1 флагом — валидация в
  `package-desktop.py`.
