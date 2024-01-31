# Steins;Hash ![Mayuri](assets/fav.ico "Mayuri")
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-pink)](https://opensource.org/license/unlicense/)

A lightweight and fast web tool powered by WebAssembly that identifies Steins;Gate episodes and timestamps from screenshots.

## How to use?
As in the demo below, just upload the image you want to search for and click 'Find Episode':



### Usage tips
- Avoid very bright or very dark images
- Images with motion blur are less likely to be found
- Try to use images that are 'identifiable': images with one or more characters, large objects, such as tables, buildings, etc.

## How it works?
The whole idea revolves around perceptual hashing. Perceptual hashing is a technique used to generate compact numerical representations, or hashes, of multimedia content based on perceptual characteristics, such as the general/structural shape of an image or sound, what a human would see as similar, rather than the raw data in itself.

These hashes, in turn, are compared with each other, if there is any similarity between them (they do not need to be exact), there is similarity between the data as well. This technique is not new and several image search engines use variations of it to find similar images.

For this repository the 'dHash' algorithm was used, as explained in '[Kind of Like That]':
> dHash tracks gradients. Here's how the algorithm works:
> 1) **Reduce size**. The fastest way to remove high frequencies and detail is to shrink the image. In this case, shrink it to 9x8 so that there are 72 total pixels. (I'll get to the "why" for the odd 9x8 size in a moment.) By ignoring the size and aspect ratio, this hash will match any similar picture regardless of how it is stretched.
> 2) **Reduce color**. Convert the image to a grayscale picture. This changes the hash from 72 pixels to a total of 72 colors. (For optimal performance, either reduce color before scaling or perform the scaling and color reduction at the same time.)
> 3) **Compute the difference**. The dHash algorithm works on the difference between adjacent pixels. This identifies the relative gradient direction. In this case, the 9 pixels per row yields 8 differences between adjacent pixels. Eight rows of eight differences becomes 64 bits.
> 4) **Assign bits**. Each bit is simply set based on whether the left pixel is brighter than the right pixel. The order does not matter, just as long as you are consistent. (I use a "1" to indicate that `P[x]` < `P[x+1]` and set the bits from left to right, top to bottom using big-endian.)

In a general, approximately 1/6th of frames (around 8.4k frames) are extracted from each episode, and a hash is computed for each of them. This process is repeated for every episode. During the search process, the hash of the query image is calculated and compared against the 216,519 stored hashes. The closest matches are then retrieved, presented in order of similarity from most to least similar.

Since the search code is done in C/WebAssembly, it is quite fast, even with such a large number of hashes.

## Indexing new episodes/animes
While this repository and site are designed for Steins;Gate, all the code provided here can be effortlessly adapted to index other anime series as well. To achieve this, simply index the new episodes as outlined below:

#### 1) Remove the current Steins;Gate index and build the index tool
```bash
$ cd Steins-Hash/
$ rm hashes

# Build the index tool
$ make index
```

#### 2) Index a single or multiple episodes
```bash
$ ./index 
Usage: ./index <video_path> <episode-number> <anime-identifier>

$ ./index foo.mp4 1 0 > hashes
```
to index multiple episodes at once, a single-line bash loop is enough:
```bash
$ for ep in {1..24}; do ./index "/path/to/MyAnime/ep$f.mp4" $f 0 >> hashes; done
```
The indexing process is quite fast and takes around ~1 minute per episode on an i5 7300HQ.

#### 3) Build everything and run
Once you have the list of hashes assembled, simply build the rest of the code with:
```bash
$ make
emcc -Wall -Wextra -O3 -s EXPORTED_FUNCTIONS='["_find_episode", "_malloc", "_free"]' find_ep.c -c
emcc -Wall -Wextra -O3 -s EXPORTED_FUNCTIONS='["_find_episode", "_malloc", "_free"]' find_ep.o -o find_ep.js
```

and run on a server of your choice, such as:
```bash
$ python -m SimpleHTTPServer (for Python 2)
# Or
$ python3 -m http.server
```

Note that everything runs on the client-side, and there is no need for additional setup such as databases, etc.

> **Note**  
To index/hash episodes, you must have [FFmpeg] and emcc ([Emscriptem SDK]) on your system and properly configured in the PATH.

## Contributing
Overall I'm pretty much satisfied with the result I got, the fact that it's a static site makes it usable in any scenario, without any database configurations etc.

However, my experience with HTML/JS/CSS is minimal to none, so any contributions in this regard are very welcome.

## License
Steins;Hash is a public domain project licensed by Unlicense.


[Kind of Like That]: https://www.hackerfactor.com/blog/?/archives/529-Kind-of-Like-That.html
[FFmpeg]: https://ffmpeg.org/download.html
[Emscriptem SDK]: https://emscripten.org/docs/getting_started/downloads.html
