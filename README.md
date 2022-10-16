# Custom Items Games plugin
This client-side plugin bypasses integrity checks for items_games.txt in Team Fortress 2. The plugin will only work locally, and it is VAC-safe. Signature scanning is used which should make the plugin work consistently after updates.

## Installation
* Download the latest release and extract the "addons" folder to TF2's main folder (../common/Team Fortress 2/tf).
* Go into tf/scripts/items, copy the items_game.txt and rename it to items_game_custom.txt. This file will be loaded instead of the original items_games.txt.
* Create a blank txt file in the same folder and name it "items_game_custom.txt.sig".
* Run the game with -insecure in the launch parameters.
* If successful, you should see the plugin loaded in console.

To disable the plugin, remove -insecure from the launch parameters. The game will not load plugins if -insecure is not present in the launch parameters, and multiplayer servers can then be joined normally. To fully uninstall, simply remove the "addons" folder.