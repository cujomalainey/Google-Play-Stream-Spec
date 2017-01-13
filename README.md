# gplay-stream-spec
Creates a real time frequency spectrum display using google play music

Purpose: Create a real time dynamic display powered by streamed music

This project takes in a google play command and generates the specturm analysis and plays the stream at the same time.

API between programs

### To C

```
play <URL>
```
Plays the given url immidietally

```
pause
```
pause the current song

```
unpause
```
resumes song

```
stop
```
stop the current song

```
quit
```
closes the process

```
color <level> <RED> <GREEN> <BLUE>
```
redefined color of a level (0-3) using RGB color code

```
volume <percent>
```
sets volume percent

```
reset
```
make sure you have the trailing space, resets bar colors and refresh rate

### From C

```
finished
```
Playback for the current song has finished