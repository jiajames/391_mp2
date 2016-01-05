	/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
	// grab the current room
    cur_room = r;
    // grab the current photo of the room
    photo_t* p = room_photo(r);
    // set the palette based on this
    set_palette(p->palette);
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    
    /*********************
	MP  ******************
	CODE  ****************
	STARTS ***************
	HERE  ****************
	*********************/ 

    //Initialize our two struct arrays and the content inside the arrays
    struct octree_array octree_level4[4096];
    struct octree_array octree_level2[64];
   
    int i, j, k;
    k = 0;

	for (i = 0; i < 4096; i++)
	{
		octree_level4[i].idx = i;
		octree_level4[i].sortedIdx = -1;
		octree_level4[i].pixelCount = 0;
		octree_level4[i].redTotal = 0;
		octree_level4[i].greenTotal = 0;
		octree_level4[i].blueTotal = 0;
	}

	for (i = 0; i < 64; i++)
	{
		octree_level2[i].idx = i;
		octree_level2[i].sortedIdx = -1;
		octree_level2[i].pixelCount = 0;
		octree_level2[i].redTotal = 0;
		octree_level2[i].greenTotal = 0;
		octree_level2[i].blueTotal = 0;
	}

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = p->hdr.height; y-- > 0; ) 
    {

		/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) 
		{

	    	/* 
		     * Try to read one 16-bit pixel.  On failure, clean up and 
		     * return NULL.
		     */
		    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
			free (p->img);
			free (p);
		        (void)fclose (in);
			return NULL;

		    }
		    /* 
		     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
		     * and 6 bits blue).  We change to 2:2:2, which we've set for the
		     * game objects.  You need to use the other 192 palette colors
		     * to specialize the appearance of each photo.
		     *
		     * In this code, you need to calculate the p->palette values,
		     * which encode 6-bit RGB as arrays of three uint8_t's.  When
		     * the game puts up a photo, you should then change the palette 
		     * to match the colors needed for that photo.
		     */

				   /*	 This code is now not necessary; we are using our own palette.
				   p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
						    (((pixel >> 9) & 0x3) << 2) |
						    ((pixel >> 3) & 0x3)); */

		// Read the pixel into the proper index according to RRRRGGGGBBBB, 4 MSBs
		i = (((pixel >> 12) << 8) | (((pixel >> 7) & 0x0F) << 4) |  ((pixel >> 1) & 0x0F));

		// Add the RGB values into the index, increment the index
		octree_level4[i].redTotal += ((pixel & 0xF800) >> 11);
		octree_level4[i].greenTotal += ((pixel >> 5) & 0x003F);
		octree_level4[i].blueTotal += (pixel & 0x1F);

		octree_level4[i].pixelCount++;
		}
	}

	// This array will link our original indeces to the sorted index
	int level4_linker[4096];

	size_t size;
	size = 4096;

	size_t size_struct;
	size_struct = sizeof(struct octree_array);

	// Sort of the level 4 array with all the pixels based on number of pixels
	qsort(octree_level4, size, size_struct, compare_pixelCount);

	// initialize level4_linker
	// this will link our original indeces (based on 4 MSBs) to our sorted index
	for (i = 0; i < 4096; i++)
		level4_linker[i] = 129;

	// linking
	for (i = 0; i < 4096; i++)
	{
		level4_linker[octree_level4[i].idx] = i;
		octree_level4[i].sortedIdx = i;	// for debugging
	}

	for (i = 0; i < 128; i++)
	{
		// Setting our first 128 palette from our level 4, if there are pixels
		//RGB values are 6 bits, thus the shifting for red and blue
		if(octree_level4[i].pixelCount != 0)
		{
			p->palette[i][0] = ((octree_level4[i].redTotal / octree_level4[i].pixelCount) << 1); 
			p->palette[i][1] = (octree_level4[i].greenTotal / octree_level4[i].pixelCount);
			p->palette[i][2] = ((octree_level4[i].blueTotal / octree_level4[i].pixelCount) << 1);
		}

		else
		{	// if no pixel count, we just initialize it (useful for debugging)
			p->palette[i][0] = 0;
			p->palette[i][1] = 0;
			p->palette[i][2] = 0;
		}
	}

	j = 0;

	// Put all pixels not yet used in the palette into proper level 2 levels
	// based on their 2 MSBS, RRGGBB. This will only look at indeces 128-4095 of level4
	for (i = 128; i < 4096; i++)
	{
		if (octree_level4[i].pixelCount != 0)
		{
			j = (((octree_level4[i].idx >> 10) & 0x03) << 4);
			j += (((octree_level4[i].idx >> 6) & 0x03) << 2);
			j += ((octree_level4[i].idx >> 2) & 0x03);

			octree_level2[j].redTotal += octree_level4[i].redTotal;
			octree_level2[j].greenTotal += octree_level4[i].greenTotal;
			octree_level2[j].blueTotal += octree_level4[i].blueTotal;
			octree_level2[j].pixelCount += octree_level4[i].pixelCount;
		}
	}

	// Inserts our 64 level 2 octtree into our palette, if there are pixels
	for (i = 128; i < 192; i++)
	{
		if (octree_level2[i - 128].pixelCount != 0)
		{
			p->palette[i][0] = ((octree_level2[i - 128].redTotal / octree_level2[i - 128].pixelCount) << 1); 
			p->palette[i][1] = (octree_level2[i - 128].greenTotal / octree_level2[i - 128].pixelCount);
			p->palette[i][2] = ((octree_level2[i - 128].blueTotal / octree_level2[i - 128].pixelCount) << 1);
		} 

		else
		{   // if no pixel count, we write 2 MSBs
			p->palette[i][0] = 0; 
			p->palette[i][1] = 0;
			p->palette[i][2] = 0;
		}
	}

	// need to reread the img files
	fseek(in, sizeof(p->hdr), SEEK_SET);
	uint8_t palette_choice;

    for (y = p->hdr.height; y-- > 0; ) 
    {
    	for (x = 0; p->hdr.width > x; x++) 
		{
			if (1 != fread (&pixel, sizeof (pixel), 1, in)) 
			{
				free (p->img);
				free (p);
		        (void)fclose (in);
			return NULL;
		    }

		    // rewrite new pixels
		    // Reads the level 4 index of the current pixel (the 4 MSBs of RGB)
		    // If it is in the first 128 indeces of our sorted level 4 octtree we write
		    // from the level 4 palette. Else we find the level 2 index and write the palette
		    // based on that index.

			i = (((pixel >> 12) << 8) | (((pixel >> 7) & 0x0F) << 4) | ((pixel >> 1) & 0x0F));

			if ((level4_linker[i] < 128) && (octree_level4[level4_linker[i]].pixelCount != 0))
			{
				palette_choice = (uint8_t)level4_linker[i];
			}

			else
			{
				i = (((pixel >> 14) << 4) | (((pixel >> 9) & 0x03) << 2) | ((pixel >> 3) & 0x0003));
				palette_choice = (uint8_t)(128 + i);
			}

			// Accounts for our 64 saved VGA indeces
		    p->img[p->hdr.width * y + x] = (palette_choice + 64);
		}
    }


    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}

/* 
 * compare_pixelCount
 *   DESCRIPTION: Used as a parameter for our qsort. Takes two array nodes
 *                a and b and sorts them based on the larger pixelCount
 *   INPUTS: const void * a - An array node in our octtree
 *	 		 const void * b - An array node in our octtree
 *   RETURN VALUE: If b > a, 1
 *				   If b = a, 0
 *				   If b < a, -1
 *   SIDE EFFECTS: Helps qsort sort based on pixelCount
 */
int compare_pixelCount(const void * a, const void * b)
{
	if (((struct octree_array*)b)->pixelCount > ((struct octree_array*)a)->pixelCount)
		return 1;
	else if (((struct octree_array*)b)->pixelCount == ((struct octree_array*)a)->pixelCount)
		return 0;
	else
		return -1;
}
