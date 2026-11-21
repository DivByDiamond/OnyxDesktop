<p align="center"><pre class="not-prose" style="text-align:center;font-family:monospace;">
    ███████
  ███▒▒▒▒▒███
 ███     ▒▒███ ████████   █████ ████ █████ █████
▒███      ▒███▒▒▒███▒▒▒███ ▒▒███ ▒███ ▒▒███ ▒▒███
▒███      ▒███ ▒███ ▒███  ▒███ ▒███  ▒▒▒█████▒
▒▒███     ███  ▒███ ▒███  ▒███ ▒███   ███▒▒▒███
 ▒▒▒███████▒   ████ █████ ▒▒███████  █████ █████
   ▒▒▒▒▒▒▒    ▒▒▒▒ ▒▒▒▒▒   ▒▒▒▒▒███ ▒▒▒▒▒ ▒▒▒▒▒
                           ███ ▒███
                          ▒▒██████
                           ▒▒▒▒▒▒
   ██████████      ██████     █████████    █████   ████      █████          ███████
  ▒▒███▒▒▒▒▒███▒  ███▒▒███   ███▒▒▒▒▒███▒ ▒▒███   ███▒      ▒▒███         ███▒▒▒▒▒███   ████ █████
   ▒███     ▒███  ▒███████  ▒███    ▒▒▒    ▒▒███  ███       ███████      ███     ▒▒███ ▒▒███ ▒▒███
   ▒███      ███  ▒███▒▒▒   ▒▒█████████    ▒███████        █▒▒▒███▒     ▒███      ▒███  ▒███  ▒███
   ▒███      ███  ▒███       ▒▒▒▒▒▒▒▒███   ▒███▒▒▒███      █  ▒███      ▒███      ▒███  █████ ████
   ▒███     ▒███  ▒███▒▒▒    ███    ▒███   ▒███ ▒▒███      █  ▒███ ███  ▒▒███     ███  ▒███▒▒▒ ▒██
   ████████████   ▒███████  ▒▒█████████    █████ ▒▒████       ▒▒█████    ▒▒▒███████▒   ▒███▒▒▒ ▒██
   ▒▒▒▒▒▒▒▒▒▒▒     ▒▒▒▒▒▒    ▒▒▒▒▒▒▒▒▒    ▒▒▒▒▒   ▒▒▒▒         ▒▒▒▒▒       ▒▒▒▒▒▒▒     ▒███▒▒▒ ▒██
</pre></p>

<p align="center"><em>Desktop environment for OnyxOS: panel, launcher and full-screen GUI applications</em></p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-yellow" alt="C99">
  <img src="https://img.shields.io/badge/target-OnyxOS-blue" alt="OnyxOS">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

----

OnyxDesktop is the desktop environment of the
[OnyxOS](https://github.com/DivByDiamond/OnyxOS) ecosystem. It holds one
program: `onyxdesk`, a ring-1 userspace daemon written in C99 and built
with the ecosystem's own compiler, OnyxCC.

`onyxdesk` claims the framebuffer for the whole session, paints a top
panel with a launcher and starts GUI applications full-screen. Managed
applications do not claim the framebuffer themselves: the desk owns it
from `DESK_READY` to `DESK_EXIT`, and the launcher comes back when an
application exits.

Input in v1 is mouse-only (`SYS_mouse_read`). Keyboard routing and
`SIGSTOP`/`SIGCONT` application switching are deferred to the kernel
input-routing milestone (M3).

----

## Session and config

The launcher is configured by `/etc/desk.conf`:

- `panel_height`: top panel height in pixels (1..200).
- `app=<name>`: one launcher entry per line; `<name>` must match a
  `/bin` binary.
- Unknown keys and blank lines are ignored.

A missing, empty or malformed file falls back to the built-in defaults
(`panel_height=24`, `osnake`, `ochat-gui`); a broken config never
crashes the daemon. The file is capped at 4096 bytes; a longer one is
truncated with a warning on stderr.

----

## Markers and hooks

Every session step prints a marker on stdout (one line, flushed), so a
boot log can be checked from outside QEMU:

- `DESK_READY`: framebuffer opened, `GET_INFO` passed, claim granted.
- `APP_STARTED <name> <pid>`: managed launch of `/bin/<name>`.
- `APP_RETURN <status>`: the application exited with `status`.
- `DESK_EXIT <code>`: session end; 0 is a clean exit, 3 means no
  framebuffer or a refused claim.

Two command-line hooks serve scripted runs:

- `--autostart NAME`: launch `/bin/NAME` right after the first panel
  paint.
- `--once`: exit after the first launched application returns
  (autostarted or picked in the launcher).

Any other argument is ignored with a note on stderr. The process exit
code is the `DESK_EXIT` code.

```sh
onyxdesk --autostart osnake --once
```

----

## Build

```sh
make ONYXCC=/path/to/OnyxCompiller/onyxcc build/onyxdesk.onx
```

The daemon is linked `--ring1`: the ring-1 gate grants `SYS_spawn` and
`SYS_kill`, which the session loop needs to manage applications. OnyxCC
links `libonyxc` automatically. CI compiles OnyxCC from source, builds
`build/onyxdesk.onx` and validates the `ONX1` magic.

----

## Tests

```sh
make -C tests/native test
```

Host test suites cover the portable modules: the config parser, panel
and launcher drawing, launcher edges, the desk loop, desk events and
framebuffer checks. They compile with plain gcc; no OnyxOS, onyxcc or
RISC-V toolchain is needed, and the suites run clean under
`gcc -fsanitize=address,undefined`. Full `.onx` builds and ONX1
validation stay in CI.

----

## OnyxOS image integration

- `OnyxOS/scripts/package-desktop.py` builds the daemon and stages
  `/bin/onyxdesk` plus `/etc/desk.conf` into the image.
- `OnyxOS/scripts/qemu-desk-smoke.sh` boots the image in QEMU and greps
  the boot log for `DESK_READY` and `APP_STARTED osnake`.

----

## Project layout

- `src/main.c`: entry point, argument parsing, config load.
- `src/config/`: `desk.conf` parser with built-in defaults.
- `src/desk/`: session loop, markers, syscall binding (`io.c`).
- `src/ui.c`, `src/font/`: panel and launcher drawing, bitmap font.
- `tests/native/`: host test suites (gcc).
- Design: `../OnyxOS/docs/dev/onyxdesktop-design.md`
- Plan and milestones: `docs/plan-onyxdesktop-impl.md`

----

## Ecosystem

| Repository | Role | Language |
|---|---|---|
| [OnyxOS](https://github.com/DivByDiamond/OnyxOS) | Ecosystem: build, disk image, docs | Bash / Python |
| [OnyxKernel](https://github.com/DivByDiamond/OnyxKernel) | RISC-V 64 kernel (Sv39, 3 rings) | Rust |
| [OnyxBoot](https://github.com/DivByDiamond/OnyxBoot) | Bootloader (M-mode, FDT) | C++20 |
| [OnyxShell](https://github.com/DivByDiamond/OnyxShell) | User shell `osh` | Rust |
| [OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) | C99 compiler `onyxcc` + linker | C99 |
| [OnyxApps](https://github.com/DivByDiamond/OnyxApps) | Userland applications | C99 |
| [OnyxDesktop](https://github.com/DivByDiamond/OnyxDesktop) | Desktop environment `onyxdesk` | C99 |

----

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

----

## Русская версия

OnyxDesktop: окружение рабочего стола экосистемы
[OnyxOS](https://github.com/DivByDiamond/OnyxOS). Репозиторий содержит
одну программу: `onyxdesk`, ring-1 демон пользовательского пространства
на C99, собираемый собственным компилятором экосистемы, OnyxCC.

`onyxdesk` занимает фреймбуфер на весь сеанс, рисует верхнюю панель с
лаунчером и запускает GUI-приложения в полный экран. Управляемые
приложения сами фреймбуфер не занимают: он принадлежит деску от
`DESK_READY` до `DESK_EXIT`, а лаунчер возвращается, когда приложение
завершается.

Ввод в v1: только мышь (`SYS_mouse_read`). Маршрутизация клавиатуры и
переключение приложений через `SIGSTOP`/`SIGCONT` отложены до вехи
маршрутизации ввода в ядре (M3).

----

## Сессия и конфигурация

Лаунчер настраивается файлом `/etc/desk.conf`:

- `panel_height`: высота верхней панели в пикселях (1..200).
- `app=<имя>`: элемент лаунчера, по одной записи на строку; `<имя>`
  должно совпадать с бинарником в `/bin`.
- Неизвестные ключи и пустые строки игнорируются.

Отсутствующий, пустой или битый файл откатывается к встроенным
значениям по умолчанию (`panel_height=24`, `osnake`, `ochat-gui`);
сломанный конфиг никогда не роняет демона. Файл ограничен 4096 байт;
более длинный обрезается с предупреждением в stderr.

----

## Маркеры и ключи запуска

Каждый шаг сеанса печатает маркер в stdout (одна строка со сбросом
буфера), поэтому загрузочный лог можно проверять снаружи QEMU:

- `DESK_READY`: фреймбуфер открыт, `GET_INFO` пройден, claim получен.
- `APP_STARTED <имя> <pid>`: управляемый запуск `/bin/<имя>`.
- `APP_RETURN <status>`: приложение завершилось со статусом `status`.
- `DESK_EXIT <code>`: конец сеанса; 0 это чистый выход, 3 значит нет
  фреймбуфера или claim отклонён.

Два ключа командной строки нужны для скриптовых прогонов:

- `--autostart NAME`: запустить `/bin/NAME` сразу после первой
  отрисовки панели.
- `--once`: выйти после возврата первого запущенного приложения
  (автозапущенного или выбранного в лаунчере).

Прочие аргументы игнорируются с заметкой в stderr. Код выхода процесса
совпадает с кодом `DESK_EXIT`.

```sh
onyxdesk --autostart osnake --once
```

----

## Сборка

```sh
make ONYXCC=/path/to/OnyxCompiller/onyxcc build/onyxdesk.onx
```

Демон линкуется с `--ring1`: гейт ring 1 даёт `SYS_spawn` и `SYS_kill`,
которые нужны циклу сеанса для управления приложениями. OnyxCC линкует
`libonyxc` автоматически. CI собирает OnyxCC из исходников, собирает
`build/onyxdesk.onx` и проверяет магию `ONX1`.

----

## Тесты

```sh
make -C tests/native test
```

Host-тестовые сюиты покрывают переносимые модули: парсер конфигурации,
отрисовку панели и лаунчера, границы лаунчера, цикл деска, события
деска и проверки фреймбуфера. Они собираются обычным gcc: не нужны ни
OnyxOS, ни onyxcc, ни RISC-V тулчейн; прогоны чистые под
`gcc -fsanitize=address,undefined`. Полная сборка `.onx` и проверка
магии ONX1 остаются в CI.

----

## Интеграция в образ OnyxOS

- `OnyxOS/scripts/package-desktop.py` собирает демон и кладёт в образ
  `/bin/onyxdesk` и `/etc/desk.conf`.
- `OnyxOS/scripts/qemu-desk-smoke.sh` загружает образ в QEMU и ищет в
  логе `DESK_READY` и `APP_STARTED osnake`.

----

## Структура проекта

- `src/main.c`: точка входа, разбор аргументов, загрузка конфига.
- `src/config/`: парсер `desk.conf` со встроенными значениями.
- `src/desk/`: цикл сеанса, маркеры, привязка сисвызовов (`io.c`).
- `src/ui.c`, `src/font/`: отрисовка панели и лаунчера, битмап-шрифт.
- `tests/native/`: host-тестовые сюиты (gcc).
- Дизайн: `../OnyxOS/docs/dev/onyxdesktop-design.md`
- План и вехи: `docs/plan-onyxdesktop-impl.md`

----

## Экосистема

| Repository | Role | Language |
|---|---|---|
| [OnyxOS](https://github.com/DivByDiamond/OnyxOS) | Ecosystem: build, disk image, docs | Bash / Python |
| [OnyxKernel](https://github.com/DivByDiamond/OnyxKernel) | RISC-V 64 kernel (Sv39, 3 rings) | Rust |
| [OnyxBoot](https://github.com/DivByDiamond/OnyxBoot) | Bootloader (M-mode, FDT) | C++20 |
| [OnyxShell](https://github.com/DivByDiamond/OnyxShell) | User shell `osh` | Rust |
| [OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) | C99 compiler `onyxcc` + linker | C99 |
| [OnyxApps](https://github.com/DivByDiamond/OnyxApps) | Userland applications | C99 |
| [OnyxDesktop](https://github.com/DivByDiamond/OnyxDesktop) | Desktop environment `onyxdesk` | C99 |

----

## Лицензия

GPL-3.0-or-later. См. [LICENSE](LICENSE).
