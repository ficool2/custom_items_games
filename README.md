# Custom Items Games plugin
This client-side plugin bypasses integrity checks for items_games.txt and proto_defs.vpd in Team Fortress 2. The plugin will only work locally, and it is VAC-safe. Signature scanning is used which should make the plugin work consistently after updates.

## Beta branches
Beta branches `pre_07_25_23_demos` and `pre_smissmas_2022_demos` currently crash on boot due to the newer item schema. This plugin supports these branches and will also fix these crashes, by forcefully disabling item schema updating.

## Installation
1. Download the latest release and extract the "addons" folder to TF2's main folder (../common/Team Fortress 2/tf).
2. If you installed this plugin just to stop the beta branches from crashing, skip to Step 5.
3. Go into tf/scripts/items, copy the items_game.txt and rename it to items_game_custom.txt. This file will be loaded instead of the original items_games.txt. proto_defs.vpd will still load the same file.
4. Create a blank txt file in the same folder and name it "items_game_custom.txt.sig".
5. Run the game with -insecure in the launch parameters.
6. If successful, you should see the plugin loaded in console.

To disable the plugin, remove -insecure from the launch parameters. The game will not load plugins if -insecure is not present in the launch parameters, and multiplayer servers can then be joined normally. To fully uninstall, simply remove the "addons" folder.