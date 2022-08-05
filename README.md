# typerace
A ncurses-powered typing game. Type the words that gradually scroll to the right as quickly as you can!

![In-game screen](img/ingame.png)

# build instructions

`make`, optionally `sudo make install` to put typeracer in PATH and install the dictionaries.

# keybinds

in main menu: use arrow keys (left, right) to cycle value, numeric entry also works. use tab/up arrow/down arrow to cycle fields. the same keybinds will work in the game over screen.

in game: type lowercase letters; they're automatically upcased. all dictionary words are upcase. words are entered automatically as you complete them, the backspace key erases the last letter. the enter key wipes out the text box. using enter to remove many characters hurts the accuracy statistic less. use F1 to prematurely terminate the game.
