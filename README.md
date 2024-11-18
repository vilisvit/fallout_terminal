
# Hacking terminal minigame from Fallout 4 (C/ncurses)
## Usage
Compilation with gcc: 
```
gcc -std=c11 -Wall -Werror fallout_terminal.c -lm -lcurses -o fallout_terminal
```

The difficulty level cold be set set with "-l" or "--level" parameter. If running the program without parameters, the easiest level would be selected.
```
./fallout_terminal -l LEVEL 
```
The game could be stopped at any time with *Ctrl-C*. (Not depending on your system default combination!)

## How to play

First, after the game is started, you need to click the mouse in any position for the program to start reading the current mouse position. Then the main screen will appear on which the whole game process takes place. According to the legend, there is a locked terminal in front of you, for which you have to guess the password.

In the center of the screen is a character block which consists of a number of random words of the same length and random characters between the words. The length and amount of words depends on the difficulty level.

On the right side of the screen is the "console" - a message block that has a command line-like design. When you click on a random word, the "console" displays the similarity (likeness) - a number that indicates the number of letter matches between the selected word and the correct one. The letters must also have the same position in the word.

The player has 4 attempts to guess the correct word. The current number of attempts is displayed at the top of the screen. Thus, with each new click on a word, the number of potential solutions decreases. When the attempts are over correct answer would appear to the "console". The program then stops after the first mouse click.
