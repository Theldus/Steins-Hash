#!/usr/bin/env python

# A Steins;Gate timestamp finder tool
# This is free and unencumbered software released into the public domain.

import sys

def remove_adjacent_duplicates(input_file, output_file):
	previous_hash = None

	with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
		for line in infile:
			if line.strip():  # Ignore empty lines
				hash_value = line.split(',')[0].split('=')[1].strip()

				if hash_value != previous_hash:
					outfile.write(line)

				previous_hash = hash_value

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("Usage: python script.py hashes new_hashes")
		sys.exit(1)

	input_file = sys.argv[1]
	output_file = sys.argv[2]

	remove_adjacent_duplicates(input_file, output_file)
