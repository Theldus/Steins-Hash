/*
 * A Steins;Gate timestamp finder tool
 * This is free and unencumbered software released into the public domain.
 */

/*
 * Index file tool, requires ffmpeg on PATH to work properly.
 *
 * This file generates 64-bit perceptive hashes for 1/6 of
 * all frames for the given video file.
 *
 * For a 24-min video file, it takes ~65s on a i5 7300HQ.
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef __TINYC__
#define STBI_NO_SIMD
#define STBIR_NO_SIMD
#endif

/* Enable if want to generate the 8x8 grayscale & the BxW version. */
#define DUMP_INTERMEDIATE_IMAGES 0

#if DUMP_INTERMEDIATE_IMAGES == 1
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#endif

/* Pixel utilities. */
#define WIDTH        9
#define HEIGHT       8
#define CHANNELS     1

#define M_intensity(i,j) \
	( ((((j)*HEIGHT)+(i))*CHANNELS)+0 )


#define PIPE_READ  0
#define PIPE_WRITE 1

#define PPM_SIZE      (WIDTH * HEIGHT)    /* Grayscale PPM without header. */
#define PPM_FULL_SIZE (11 + (PPM_SIZE*3)) /* Header + 9*8 pixels in RGB.   */

static uint8_t ppm_buff[PPM_SIZE];

struct ffmpeg
{
	int pipe_fd;
	pid_t pid;
};

/**
 * @brief Exhibits a formatted message on stderr and aborts
 * the program.
 *
 * @param fmt Formatted message, just like printf.
 */
static inline void panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

/**
 * Simple perceptual hash algorithm:
 * - Resize to 8x8 (already done)
 * - Grayscale (already done)
 * - Convert to BxW through average intensity and them create the hash.
 *
 * @return Returns a 64-bit hash that corresponds to the input file.
 */
static uint64_t perceptual_hash(void)
{
	uint64_t hash;

#if DUMP_INTERMEDIATE_IMAGES == 1
	stbi_write_jpg("img88gray.jpg", WIDTH, HEIGHT, 1, ppm_buff, 100);
#endif

	/* Compute hash. */
	hash = 0;
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH - 1; j++) {
			int intensityL = ppm_buff[M_intensity(i,j)];
			int intensityR = ppm_buff[M_intensity(i,j+1)];

			int set = 0;
			if (intensityL < intensityR)
				set = 1;

			hash = (hash << 1) | set;
		}
	}

	return (hash);
}

/**
 * Safe string-to-int routine that takes into account:
 * - Overflow and Underflow
 * - No undefined behaviour
 *
 * Taken from https://stackoverflow.com/a/12923949/3594716
 * and slightly adapted: no error classification, because
 * I dont need to know, error is error.
 *
 * @param out Pointer to integer.
 * @param s String to be converted.
 *
 * @return Returns 0 if success and a negative number otherwise.
 */
static int str2int(int *out, char *s)
{
	char *end;
	if (s[0] == '\0' || isspace(s[0]))
		return (-1);
	errno = 0;

	long l = strtol(s, &end, 10);

	/* Both checks are needed because INT_MAX == LONG_MAX is possible. */
	if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
		return (-1);
	if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
		return (-1);
	if (*end != '\0')
		return (-1);

	*out = l;
	return (0);
}

/**
 * @brief Start ffmpeg as a child process and then creates
 * a pipe to transfer all the extracted frames to this program.
 *
 * @param ff   FFmpeg structure.
 * @param file Video file to be read.
 */
static void ffmpeg_start(struct ffmpeg *ff, const char *file)
{
	int pipes[2];

	if (pipe(pipes) < 0)
		panic("Unable to create a pipe!\n");

	if ((ff->pid = fork()) < 0)
		panic("Unable to fork!\n");

	if (ff->pid == 0) {
		if (dup2(pipes[PIPE_WRITE], STDOUT_FILENO) < 0)
			panic("Unable to dup2 read!\n");

		close(pipes[PIPE_READ]);

		/*
		 * Ouput 6 frames/sec in the stdout as PPM images
		 * at 9x8, grayscale
		 */
		int ret = execlp("ffmpeg",
			"ffmpeg",
			"-loglevel", "fatal",
			"-i",        file,
			"-vf",       "scale=9:8,format=gray,fps=6",
			"-f",        "image2pipe",
			"-vcodec",   "ppm",
			"-",
			NULL
		);

		if (ret < 0)
			panic("Error while running ffmpeg!\n");
	}

	ff->pipe_fd = pipes[PIPE_READ];
	close(pipes[PIPE_WRITE]);
}

/**
 * @brief Given a file descritor @p fd, a buffer @p buff,
 * and @p amnt, reads all its content.
 *
 * @param fd   File descriptor to be read.
 * @param buff Target buffer.
 * @param amnt Amnt of bytes to be read.
 *
 * @return Rturns 1 if success, -1 otherwise.
 */
static inline int read_all(int fd, uint8_t *buff, size_t amnt)
{
	uint8_t *p = buff;
	ssize_t r;
	while (amnt > 0)
	{
		r = read(fd, p, amnt);
		if (r <= 0)
			return (-1);

		p    += r;
		amnt -= r;
	}
	return (1);
}

/**
 * @brief Read the next PPM image from the FFmpeg pipe.
 *
 * @param ff FFMpeg structure.
 *
 * @return Returns 1 if success, 0 otherwise (or EOF).
 */
static int ffmpeg_next_image(struct ffmpeg *ff)
{
	uint8_t *p;
	uint8_t full_image[PPM_FULL_SIZE] = {0};

	p = full_image;

	/* Read the entire image (header+RGB content). */
	if (read_all(ff->pipe_fd, full_image, sizeof full_image) < 0)
		return (0);

	/* Skip entire header (11 bytes): "P6\n9 8\n255\n" */
	p += 11;

	/*
	 * Convert our input from 3 bytes-per-pixel to
	 * 1 byte-per-pixel. (The image is already in grayscale)
	 */
	for (int i = 0, j = 0; i < 9*8*3; i += 3, j++)
		ppm_buff[j] = p[i];

	return (1);
}

/* Main routine. */
int main(int argc, char **argv)
{
	int ident;
	int episode;
	uint64_t hash;
	uint32_t fcount;
	struct ffmpeg ff;

	if (argc < 4) {
		panic("Usage: %s <video_path> <episode-number>"
			" <anime-identifier>\n", argv[0]);
	}

	if (str2int(&episode, argv[2]) < 0)
		panic("Malformed episode number!\n");
	if (str2int(&ident, argv[3]) < 0)
		panic("Malformed identifier!\n");

	ffmpeg_start(&ff, argv[1]);

	fcount = 0;
	while (ffmpeg_next_image(&ff))
	{
		hash = perceptual_hash();
		printf(
			"{"
			".hash = 0x%016" PRIx64 ", "
			".frame    = %u, "
			".episode  = %u, "
			".anime_id = %u  "
			"},\n",
			hash, fcount++, episode, ident);
	}

	/* Close ffmpeg if not yet. */
	close(ff.pipe_fd);
	waitpid(ff.pid, NULL, 0);
	return (0);
}
