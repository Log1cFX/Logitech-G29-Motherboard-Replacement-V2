// Run Without Debugging (Ctrl+F5) on an ST-LINK debug configuration (type
// "stlinkgdbtarget"). ST's debug adapter ignores "noDebug" and would start a
// full debug session: this cancels it and runs the flash task of the
// workspace instead. Start Debugging (F5) is left as it is
"use strict";

const vscode = require("vscode");

// Called before each ST-LINK session starts, before the variables of its
// configuration are substituted: a cancelled run evaluates none of them, so
// its preBuild does not build twice
async function resolveDebugConfiguration(folder, config) {
  if (!config.noDebug) {
    return config;
  }
  const task = vscode.workspace
    .getConfiguration("flashOnRun", folder)
    .get("task", "Flash");
  // By its label, as Terminal > Run Task does
  await vscode.commands.executeCommand("workbench.action.tasks.runTask", task);
  // No debug session, and no error
  return undefined;
}

function activate(context) {
  context.subscriptions.push(
    vscode.debug.registerDebugConfigurationProvider("stlinkgdbtarget", {
      resolveDebugConfiguration,
    })
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
