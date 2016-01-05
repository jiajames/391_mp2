#ifndef OCTREE_H
#define OCTREE_H

struct octree_array
{
	int idx;
	int sortedIdx;
	int pixelCount;
	int redTotal;
	int greenTotal;
	int blueTotal;
};

void level2_init(struct octree_array * level2);