# Flash on Run Without Debugging

A VS Code extension for the firmware workspace (`app/`). ST's debug adapter
ignores Run Without Debugging (Ctrl+F5) and starts a full debug session. With
this extension, Ctrl+F5 on the ST-LINK configuration of `.vscode/launch.json`
runs the `Flash` task of `.vscode/tasks.json` instead: build, flash the
bootloader and the firmware of the active preset, reset. Start Debugging (F5)
is unchanged.

## Install, once per machine

Command Palette → **Developer: Install Extension from Location...** → select
this folder, then **Developer: Reload Window**. After a change to
`extension.js`, install it the same way again.

## Setting

`flashOnRun.task`: label of the task to run, `Flash` by default.
