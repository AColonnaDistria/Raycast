## Raycast
A simple raycasting engine written in C using SDL2 using the digital differential analyzer algorithm. This project displays a dual-view interface: a top-down 2D map on the left and the 3D rendered perspective on the right. You can move up, down, left and right by using the ZQSD keys on your keyboard, and rotating left and right by using W and X.

## Build
This project needs SDL2 in order to be able to run.

```
sudo apt install libsdl2-2.0-0 libsdl2-gfx-1.0-0 libsdl2-image-2.0-0 libsdl2-mixer-2.0-0 libsdl2-net-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-dev
```

And then compile the project:

```
gcc main.c labyrinth.c -o raycast $(sdl2-config --cflags --libs) -lSDL2_image -lm
```

## Video

[![Watch the video](https://img.youtube.com/vi/EMSSQnqssfk/maxresdefault.jpg)](https://youtu.be/EMSSQnqssfk)

## Illustrations

![alt text](readme-images/image1.png "Illustration")
