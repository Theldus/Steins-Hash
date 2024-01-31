# Steins;Hash ![Mayuri](assets/fav.ico "Mayuri")
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-pink)](https://opensource.org/license/unlicense/)

A lightweight and fast web tool powered by WebAssembly that identifies anime episodes and timestamps from screenshots.

## How it works?
The whole idea revolves around perceptual hashing. Perceptual hashing is a technique used to generate compact numerical representations, or hashes, of multimedia content based on perceptual characteristics, such as the general/structural shape of an image or sound, what a human would see as similar, rather than the raw data in itself.

These hashes, in turn, are compared with each other, if there is any similarity between them (they do not need to be exact), there is similarity between the data as well. This technique is not new and several image search engines use variations of it to find similar images, although the algorithm itself is never properly disclosed.

For this repository 'dHash' was used, as explained in '[Kind of Like That]':
> dHash tracks gradients. Here's how the algorithm works:
> 1) **Reduce size**. The fastest way to remove high frequencies and detail is to shrink the image. In this case, shrink it to 9x8 so that there are 72 total pixels. (I'll get to the "why" for the odd 9x8 size in a moment.) By ignoring the size and aspect ratio, this hash will match any similar picture regardless of how it is stretched.
> 2) **Reduce color**. Convert the image to a grayscale picture. This changes the hash from 72 pixels to a total of 72 colors. (For optimal performance, either reduce color before scaling or perform the scaling and color reduction at the same time.)
> 3) **Compute the difference**. The dHash algorithm works on the difference between adjacent pixels. This identifies the relative gradient direction. In this case, the 9 pixels per row yields 8 differences between adjacent pixels. Eight rows of eight differences becomes 64 bits.
> 4) **Assign bits**. Each bit is simply set based on whether the left pixel is brighter than the right pixel. The order does not matter, just as long as you are consistent. (I use a "1" to indicate that `P[x]` < `P[x+1]` and set the bits from left to right, top to bottom using big-endian.)

In a general, approximately 1/6 of frames (around 8.4k frames) are extracted from each episode, and a hash is computed for each of them. This process is repeated for every episode. During the search process, the hash of the query image is calculated and compared against the 216,519 stored hashes. The closest matches are then retrieved, presented in order of similarity from most to least similar.

Since the search code is done in C/WebAssembly, it is quite fast, even with such a large number of hashes.





[Kind of Like That]: https://www.hackerfactor.com/blog/?/archives/529-Kind-of-Like-That.html
