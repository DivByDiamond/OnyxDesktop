# OnyxDesktop M2 — план реализации

> **For agentic workers:** REQUIRED SUB-SKILL: subagent-driven-development
> (или executing-plans). Шаги — чекбоксы (`- [ ]`).

**Goal:** минимальный рабочий стол: бар с часами, фото-виджеты,
консоль-виджет (osh), лаунчер-оверлей по бинду, конфиг биндов.

**Architecture:** расширение v1: стейт-машина desktop↔overlay в desk.c,
модули kbd/widgets, все системные вызовы через швы desk_io, TDD на host.

**Tech Stack:** C99 + libonyxc, onyxcc (ring 1), host-тесты gcc, QEMU.

**Spec:** `../OnyxOS/docs/dev/onyxdesktop-m2-design.md` (2026-11-21).

## Global Constraints

- Контракты v1 не ломать: маркеры `DESK_READY/APP_STARTED/APP_RETURN/DESK_EXIT`
  ровно один раз; claim весь сеанс; RELEASE один на `out:`; ошибки fb → 3;
  `--autostart/--once` работают; `qemu-desk-smoke.sh` зелёный без правок
  скрипта (маркеры те же).
- ≤200 строк/файл, ≤4 файла/папку (подпапки отдельно); тесты отдельными
  файлами; мёртвого кода нет.
- Новые строки без «—», без ИИ-паттернов (стиль репо).
- Коммиты `feat:`/`fix:`/`docs:`, явные пути, push, без ИИ/агентов в тексте.

## Review Focus

1. Битый/чужой BMP (кривой заголовок, сжатие, выход за кадр) → пропуск с
   предупреждением, не падение и не выход за буфер (guard-зона в тестах).
2. Клавиша бинда во время работы приложения не должна попасть в desk
   (termios восстановлен; desk в waitpid). Тест: сценарий autostart с
   инжекцией ключа до wait → ключ не диспетчерится.
3. Консоль: смерть osh (spawn-отказ) → виджет закрывается, стол живёт;
   kill при закрытии стола (выход с открытым виджетом) → waitpid, без
   зомби (счётчик в фейке).
4. Переполнение конфига: >4 photo, неизвестный bind-ключ, кривые
   координаты → игнор с stderr, дефолтные бинды целы.
5. poll/pipe на host-фейке: консольный вывод НЕ читается как TTY (швы
   разделены), ничего не блокирует основной цикл (фейк считает таймеры).

---

### Task 1: Конфиг M2: binds + photos

**Files:** Modify: `src/config/config.{c,h}`; Test: `tests/native/config/test_config.c`
**Interfaces (в struct desk_config):**
```c
char bind_key[2];   /* [0]=launcher, [1]=console: esc=27, tab=9, иначе ASCII */
char photos[DESK_MAX_PHOTOS][DESK_PATH_MAX]; int photo_x[..], photo_y[..]; int n_photos;
```
`DESK_MAX_PHOTOS 4`, `DESK_PATH_MAX 64`; формат строк — в спеке.
- [x] RED: parse binds (валидные/невалидные/дубли), photos (лимит 4,
  путь без запятой в имени, x/y отрицательные/кривые), дефолты
  launcher:esc console:tab. 2026-11-21: новый
  tests/native/config/test_binds.c не собирается против старого
  struct desk_config (нет bind_key/photos/DESK_KEY_*).
- [x] GREEN; все старые 57+ проверок целы. 2026-11-21: test_config 57
  + test_binds 49 checks, 0 failures; парсинг bind/photo вынесен в
  src/config/bind.{c,h} (config.c 164 строки, лимит 200);
  build/onyxdesk.onx ONX1; ASan/UBSan чистый.

### Task 2: kbd — raw TTY и диспетчер биндов

**Files:** Create: `src/desk/kbd/{kbd.c,kbd.h}`; Modify: `src/desk/desk.{c,h}`
(швы `tty_raw/tty_restore/tty_poll/kbd_read` в desk_io), `src/desk/io.c`
(целевые биндинги termios/poll), тесты `tests/native/kbd/`.
- [x] RED: инжекция ключа launcher в сценарии desktop → действие отмечено
  (счётчик через фейк), stderr-хук залогирован; ключ не в биндах → игнор;
  TTY raw после CLAIM, restore перед spawn, raw после wait, ноль чтений
  между spawn и wait. 2026-11-21: tests/native/kbd/test_kbd.c не
  собирается против старого дерева (нет src/desk/kbd/kbd.{c,h}).
- [x] GREEN; сборка .onx ONX1 ring1. 2026-11-21: 308+45 checks, 0
  failures (test_kbd 45); ASan/UBSan чистый; kbd в подпапке
  src/desk/kbd (лимит 4 файла/папку), desk.c 199 строк (tty-хелперы
  kbd_arm/kbd_yield/kbd_reclaim вынесены в kbd.c); отказ raw →
  тихая деградация (мышиный стол, заявка stderr).

### Task 3: BMP + фото-виджет

**Files:** Create: `src/widgets/bmp.{c,h}`, `src/widgets/photo.{c,h}`; Test:
`tests/native/bmp/`.
- [x] RED: декодер на синтетических буферах (24/32-бита, bottom-up,
  кривая магия/сжатие → ошибка, клип у края кадра, guard-зона).
  2026-11-21: tests/native/bmp не собирается против старого дерева
  (нет src/widgets/bmp.{c,h}, photo.{c,h}, ui_rgb/ui_put_px).
- [x] GREEN; тест-фикстура BMP (генерится в тесте, не коммитится).
  2026-11-21: test_bmp 65 + test_photo 32 checks, 0 failures; весь
  набор 450 checks (было 353); ui_put_px/ui_frame_ok/ui_rgb
  экспортированы из ui.h (put_px-дисциплина без дублирования);
  шов read_file в desk_io (target: open/read/close, host-фейк по
  реестру), cap фото 64 KiB + 1 байт запаса на определение
  переполнения; ASan/UBSan чистый (поймал overread в тестовом фейке
  read_file, исправлен); build/onyxdesk.onx ONX1, flags 0x2 (ring 1
  по определению ядра).

### Task 4: бар-виджеты: часы/uptime

**Files:** Create: `src/widgets/bar.{c,h}`; Modify: `src/desk/io.c` (шов
`clock`), тесты.
- [ ] RED: год>2020 → HH:MM:SS; иначе uptime; NULL-шов → пусто.
- [ ] GREEN.

### Task 5: консоль-виджет

**Files:** Create: `src/widgets/console.{c,h}`; Modify: desk_io (+pipe/
spawn_dup/kill/waitpid-швы), desk.c (состояние), тесты `tests/native/console/`.
- [ ] RED: открытие (pipe+spawn оsh), вывод рендерится в рамке (фейк
  возвращает строки), ввод проксируется, закрытие → kill+wait ровно
  один раз, spawn-отказ → виджет закрыт, стол жив, выход стола с открытым
  виджетом → kill на `out:`.
- [ ] GREEN.

### Task 6: стейт-машина стола + лаунчер-оверлей

**Files:** Modify: `src/desk/desk.c` (≤200: вынести переполнение в
`src/desk/overlay.{c,h}` при необходимости), `src/ui.c` (рамка оверлея);
тесты desk_events.
- [ ] RED: desktop → bind → overlay → клик мимо → desktop; клик по пункту
  → spawn (маркеры как v1); клик по пункту при once → DESK_EXIT 0.
- [ ] GREEN; ASan/UBSan; сборка; `--autostart osnake --once` сценарий
  маркерами не отличается от v1.

### Task 7: интеграция, скриншот, доки

**Files:** Modify: OnyxOS `etc/desk.conf`-шаблон в OnyxDesktop
(`etc/desk.conf` + `etc/desk-logo.bmp` маленький образец), smoke-прогон,
README (EN+RU) обоих репо, снимок нового стола в OnyxOS
`docs/img/screenshot.png`.
- [ ] `qemu-desk-smoke.sh` PASS без правок скрипта. (при smoke глянуть
  память ring-1: статический фото-буфер 64 KiB в .bss,
  src/widgets/photo.c)
- [ ] Новый скриншот: бар + фото + лаунчер-оверлей; README обновлён.
- [ ] OnyxOS `docs/dev/roadmap.md`: строка про OnyxDesktop M2 (бар-виджеты,
  фото, консоль, бинды) в актуальный раздел + `OnyxKernel/todo.md` P2 при
  необходимости.
- [ ] Commits `feat: ...` по репо, push.
