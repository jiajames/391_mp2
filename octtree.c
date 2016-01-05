#include <stdio.h>
#include <stdlib.h>

void level2_init(struct octree_array * level2)
{
	int i;
	for (i = 0; i < 64; i++)
	{
		level2[i].idx = i;
		level2[i].sortedIdx = -1;
		level2[i].pixelCount = 0;
		level2[i].redTotal = 0;
		level2[i].greenTotal = 0;
		level2[i].blueTotal = 0;
	}
}

void level4_init(struct octree_array * level4)
{
	int i;
	for (i = 0; i < 4096; i++)
	{
		level4[i].idx = i;
		level4[i].sortedIdx = -1;
		level4[i].pixelCount = 0;
		level4[i].redTotal = 0;
		level4[i].greenTotal = 0;
		level4[i].blueTotal = 0;
	}
}

void read_pixel(uint16_t pixel, octree_array * octree_level4)
{
	int i;

	i = (0x100 * (pixel >> 12)); // red
	i += (0x010 * (pixel >> 7) & 0x0F); // green
	i += ((pixel >> 2) & 0x0F); // blue

	octree_level4.redTotal = ((pixel & 0xF800) >> 11);
	octree_level4.greenTotal = ((pixel & 0x7E0) >> 5);
	octree_level4.blueTotal = (pixel & 0x1F);

	octree_level4.pixelCount++;
}

int compare_pixelCount(const void * a, const void * b)
{
	return (b->pixelCount - a->pixelCount);
}

void new_palette_level4(octree_array * octree_level4, uint8_t palette[192][3], int * level4_linker)
{
	int i, originalIdx;

	qsort(octree_level4, 4096, sizeof(struct octree_array), compare_pixelCount);

	// forward old indeces to new indeces after sort
	for (i = 0; i < 4096; i++)
	{
		level4_linker[octree_level4[i].idx] = i;
	}

	for (i = 0; i < 4096; i++)
	{
		octree_level4[i].sortedIdx = i;
	}

	for (i = 0; i < 128; i++)
	{
		if(octree_level4[i].pixelCount != 0)
		{
			palette[i][0] = octree_level4[i].redTotal / octree_level4[i].pixelCount; 
			palette[i][1] = octree_level4[i].greenTotal / octree_level4[i].pixelCount;
			palette[i][2] = octree_level4[i].blueTotal / octree_level4[i].pixelCount;
		}

		else
		{
			
		}
	}
}

void read_pixel_level2(octree_array * octree_level4, octree_array * octree_level2)
{
	int i, j;

	for (i = 128; i < 4096; i++)
	{
		if (octree_level4[i].pixelCount != 0)
		{
			j = (((octree_level4[i].idx >> 10) & 0x03) * 0x010);
			j += (((octree_level[i].idx >> 6) & 0x03) * 0x04);
			j += ((octree_level[i].idx >> 2) & 0x03);

			octree_level2[j].redTotal = octree_level4[i].redTotal;
			octree_level2[j].greenTotal = octree_level4[i].greenTotal;
			octree_level2[j].blueTotal = octree_level4[i].blueTotal;
			octree_level2[j].pixelCount = octree_level4[i].pixelCount;
		}
	}
}

void new_pallete_level2(octree_array * octree_level2, uint8_t palette[192][3])
{
	for (i = 128; i < 196; i++)
	{
		if (octree_level2[i].pixelCount != 0)
		{
			palette[i][0] = octree_level2[i].redTotal / octree_level4[i].pixelCount; 
			palette[i][1] = octree_level2[i].greenTotal / octree_level4[i].pixelCount;
			palette[i][2] = octree_level2[i].blueTotal / octree_level4[i].pixelCount;
		}

		else
		{

		}
	}
}

uint8_t choose_palette(uint16_t pixel, int * level4_linker)
{
	int i;

	i = (0x100 * (pixel >> 12)); // red
	i += (0x010 * (pixel >> 7) & 0x0F); // green
	i += ((pixel >> 2) & 0x0F); // blue

	if (level4_linker[i] < 128)
	{
		return (uint8_t)level4_linker[i];
	}

	else
	{
		i = ((pixel >> 14) * 0x010);
		i += (((pixel >> 9) & 0x03) * 0x04);
		i += ((pixel >> 3) & 0x03);

		return (uint8_t)(128 + i);
	}
}