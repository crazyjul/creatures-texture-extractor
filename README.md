# creatures-texture-extractor

This tools extract Creatures 3 textures to png.

## How to build

This tools uses cmake and conan to build. Assuming you have both available in your path, you can build using this command

```
mkdir build
cmake -G "Unix Makefiles"
cd build && make
```

### Usage

You should provide the path of image as first argument, the second argument is the output image.

```
texture_extractor machine.c16 machine.png
```

If the image is either a blk or contains multiple image, the output path must contain `%i`. It will be replaced by the texture index.

```
texture_extractor background.blk background%i.png
```