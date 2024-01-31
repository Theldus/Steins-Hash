/*
 * A Steins;Gate timestamp finder tool
 * This is free and unencumbered software released into the public domain.
 */

/* WebAsm code that is called from JS (although works just fine
 * without any JS). */

#include <inttypes.h>
#include <stdio.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

struct phash
{
	uint64_t hash;
	uint16_t frame;    /* Frame number.   */
	uint16_t episode;  /* Episode number. */
	uint8_t  anime_id; /* Which anime?    */
} __attribute__((packed)) ;

static struct phash anime_hashes[] = {
	#include "hashes"
};

#define TBL_SIZE (sizeof(anime_hashes)/sizeof(anime_hashes[0]))

#define WIDTH        9
#define HEIGHT       8
#define CHANNELS     1
#define MAX_DISTANCE 10

#define NUM_FOUND 20

#define M_intensity(i,j) \
	( ((((j)*HEIGHT)+(i))*CHANNELS)+0 )

/**
 * Simple perceptual hash algorithm:
 * - Resize to 8x8 (already done)
 * - Grayscale (already done)
 * - Convert to BxW through average intensity and them create the hash.
 *
 * @param img Input image buffer in RGBA
 *
 * @return Returns a 64-bit hash that corresponds to the input file.
 */
static uint64_t perceptual_hash(uint8_t *img)
{
	uint8_t  gray[WIDTH * HEIGHT] = {0};
	uint64_t hash;

	/*
	 * Convert to gray scale img first.
	 * (JS image is RGBA)
	 */
	for (int i = 0, j = 0; i < WIDTH*HEIGHT*4; i += 4, j++)
		gray[j] = (img[i] + img[i+1] + img[i+2])/3;

	/* Compute hash. */
	hash = 0;
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH - 1; j++) {
			int intensityL = gray[M_intensity(i,j)];
			int intensityR = gray[M_intensity(i,j+1)];

			int set = 0;
			if (intensityL < intensityR)
				set = 1;

			hash = (hash << 1) | set;
		}
	}

	return (hash);
}

/**
 * @brief Calculates the hamming distance between
 * two 64-bit values.
 *
 * @param n1 First number.
 * @param n2 Second number.
 *
 * @return Return the number of bits set.
 */
static inline int hamming(uint64_t n1, uint64_t n2)
{
	uint64_t x = n1 ^ n2;
	return __builtin_popcountll(x);
}

/* Results structure with the same size as the anime hashes table. */
struct result {
	int distance;    /* Hamming distance. */
	uint32_t index;  /* Table index.      */
} results[TBL_SIZE];

/* Compares two distances */
static int
cmp_distance(const void *p1, const void *p2)
{
	struct result *r1, *r2;
	r1 = (struct result *)p1;
	r2 = (struct result *)p2;
	return (r1->distance - r2->distance);
}

/**
 * @brief Given an image in @p img with @p width and @height,
 * calculate its hash and then search for the closests
 * images.
 *
 * @param img      Image buffer in RGBA.
 * @param width    Image width.
 * @param height   Image height.
 * @param frames   List of frame number that are similar to
 *                 the reference image.
 * @param episodes Episode number list.
 * @param anime_id Anime identifier that belongs the returned
 *                 frames and episodes.
 */
void find_episode(uint8_t *img, int width, int height,
	int32_t *frames, int32_t *episodes, int32_t *anime_id)
{
	int ret;
	size_t i, j;
	int distance;
	uint64_t hash;
	uint8_t resized[9 * 8 * 4];

	/* Fill our table first with invalid distances. */
	for (i = 0; i < TBL_SIZE; i++)
		results[i].distance = 255;

	/*
	 * Resize image using STB, because the HTML Canvas resize
	 * generates very poor images!
	 */
	ret = stbir_resize_uint8(img, width, height, 0,
		resized, WIDTH, HEIGHT, 0, 4);

	if (!ret) {
		printf("Unable to resize!\n");
		return;
	}

	hash = perceptual_hash(resized);
	printf("Hash: %016" PRIx64 "\n", hash);

	/* Now loop through our hash list calculating the
	 * hamming distance to every frame, and if within
	 * the threshold, we found.
	 */
	for (i = 0, j = 0; i < TBL_SIZE; i++) {
		distance = hamming(hash, anime_hashes[i].hash);
		if (distance >= MAX_DISTANCE)
			continue;

		results[j].distance = distance;
		results[j].index = i;
		j++;
	}

	/* If none is found, then skip everything. */
	if (!j) {
		printf("Not found!\n");
		return;
	}

	/* Sort by distance. */
	qsort(results, TBL_SIZE, sizeof(struct result), cmp_distance);

	/* Iterate over the nearest frames and save them. */
	for (i = 0; i < NUM_FOUND; i++) {
		/* Abort if found an invalid entry. */
		if (results[i].distance == 255)
			break;

		uint32_t idx = results[i].index;
		frames[i]    = anime_hashes[idx].frame;
		episodes[i]  = anime_hashes[idx].episode;
		anime_id[i]  = anime_hashes[idx].anime_id;

		printf("Found!: %016" PRIx64 ", frame: %d, ep: %d, id: %d"
			" (distance: %d)\n",
			anime_hashes[idx].hash,
			frames[i],
			episodes[i],
			anime_id[i],
			results[i].distance);
	}
}
