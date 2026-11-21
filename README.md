# OnyxDesktop

Desktop environment and window manager for [Onyx OS](https://github.com/DivByDiamond/OnyxOS).

`onyxdesk` is a ring-1 userspace daemon that owns the framebuffer
session: it draws a panel and a launcher, starts GUI applications
full-screen and returns to the launcher when they exit. v1 uses only
the mouse (`SYS_mouse_read`); keyboard routing and `SIGSTOP`/`SIGCONT`
switching arrive with the kernel input-routing work (M3).

## Build

```sh
make ONYXCC=/path/to/OnyxCompiller/onyxcc build/onyxdesk.onx
make -C tests/native test
```

## Design and plan

- Design: `../OnyxOS/docs/dev/onyxdesktop-design.md`
- Plan: `docs/plan-onyxdesktop-impl.md`

## Status

M0 skeleton — see the plan's milestones.
