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
- [ ] RED: parse binds (валидные/невалидные/дубли), photos (лимит 4,
  путь без запятой в имени, x/y отрицательные/кривые), дефолты
  launcher:esc console:tab.
- [ ] GREEN; все старые 57+ проверок целы.

### Task 2: kbd — raw TTY и диспетчер биндов

**Files:** Create: `src/desk/kbd.{c,h}`; Modify: `src/desk/desk.h` (швы:
`tty_raw/tty_restore/kbd_poll` в desk_io), `tests/native/desk/` (+подпапка
`tests/native/kbd/`).
- [ ] RED: инжекция ключа launcher в сценарии desktop → открытие
  оверлея (счётчик перерисовок); ключ не в биндах → игнор; TTY raw
  ставится после CLAIM, снимается перед spawn.
- [ ] GREEN; сборка .onx ONX1 ring1.

### Task 3: BMP + фото-виджет

**Files:** Create: `src/widgets/bmp.{c,h}`, `src/widgets/photo.{c,h}`; Test:
`tests/native/bmp/`.
- [ ] RED: декодер на синтетических буферах (24/32-бита, bottom-up,
  кривая магия/сжатие → ошибка, клип у края кадра, guard-зона).
- [ ] GREEN; тест-фикстура BMP (генерится в тесте, не коммитится).

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
- [ ] `qemu-desk-smoke.sh` PASS без правок скрипта.
- [ ] Новый скриншот: бар + фото + лаунчер-оверлей; README обновлён.
- [ ] Commits `feat: ...` по репо, push.
