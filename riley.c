#include "riley.h"

#include <assert.h>
#include <stdbool.h>

c16_t*
c16_new (c16_format_t format)
{
  c16_t *image = malloc (sizeof (c16_t)); {
    image->format = format;
    image->count = 0;
    image->sprites = NULL;
  }
  return image;
}

c16_format_t
c16_get_format (c16_t* image)
{
  return image->format;
}

c16_sprite_t
c16_sprite_make (uint16_t width, uint16_t height, uint16_t* data)
{
  c16_sprite_t sprite = { width, height, data };
  return sprite;
}

uint16_t
c16_get_number_of_sprites (c16_t *image)
{
  return image->count;
}

void
c16_set_number_of_sprites (c16_t *image, uint16_t count)
{
  image->count = count;
  image->sprites = realloc (image->sprites, count * sizeof (c16_sprite_t));
}

c16_sprite_t
c16_get_sprite (c16_t *image, uint16_t index)
{
  assert (index < image->count);
  return image->sprites[index];
}

void
c16_set_sprite (c16_t *image, uint16_t index, c16_sprite_t sprite)
{
  assert (index < image->count);
  image->sprites[index] = sprite;
}

uint16_t*
c16_sprite_get_data (c16_sprite_t sprite)
{
  return sprite.data;
}

uint16_t
c16_sprite_get_width (c16_sprite_t sprite)
{
  return sprite.width;
}

uint16_t
c16_sprite_get_height (c16_sprite_t sprite)
{
  return sprite.height;
}

c16_t*
c16_new_from_file (FILE *fp)
{
  c16_t* image;

  uint32_t header;
  bool is_565;
  uint16_t count;
  
  assert (fread (&header, 1, 4, fp) == 4);
  
  is_565 = header & 0x1;
  assert (header & 0x2); // Is a c16_t File
  
  image = c16_new (is_565? C16_565 : C16_555);
  
  assert (fread (&count, 1, 2, fp) == 2);
  c16_set_number_of_sprites (image, count);
  
  for (int i = 0; i < count; ++i) {
    c16_sprite_t sprite;
    uint32_t first_line;
    uint16_t width, height;
    char *data = NULL;
    uint16_t *data_pos = NULL;
    int j;
    
    assert (fread (&first_line, 1, 4, fp) == 4);
    assert (fread (&width, 1, 2, fp) == 2);
    assert (fread (&height, 1, 2, fp) == 2);
    
    data = malloc (width * height * 2);
    data_pos = (uint16_t*)data;
    assert (data);
    
    uint32_t offsets[height];
    offsets[0] = first_line;
    for (j = 1; j < height; ++j)
      assert (fread (&offsets[j], 1, 4, fp) == 4);
    
    long mark = ftell (fp);
    
    for (j = 0; j < height; ++j) {
      fseek (fp, offsets[j], SEEK_SET);
      while (true) {
        uint16_t tag;
        assert (fread (&tag, 1, 2, fp) == 2);
        if (tag == 0) break;
        bool color = tag & 0x0001;
        uint16_t length = (tag & 0xFFFE) >> 1;
        if (color) {
          assert (fread (data_pos, 1, length * 2, fp) == length * 2);
        } else {
          memset (data_pos, 0, length * 2);
        }
        data_pos += length;
      }
    }
    
    sprite = c16_sprite_make (width, height, (uint16_t*)data);
    c16_set_sprite (image, i, sprite);
    
    fseek (fp, mark, SEEK_SET);
  }
  
  fclose (fp);
  
  return image;
}

c16_t*
s16_new_from_file (FILE *fp)
{
  c16_t* image;

  uint32_t header;
  bool is_565;
  uint16_t count;
  
  assert (fread (&header, 1, 4, fp) == 4);
  
  is_565 = header & 0x1;
  assert (header ^ 0x2); // Is an S16 File; not c16_t
  
  image = c16_new (is_565? C16_565 : C16_555);
  
  assert (fread (&count, 1, 2, fp) == 2);
  c16_set_number_of_sprites (image, count);
  
  for (int i = 0; i < count; ++i) {
    c16_sprite_t sprite;
    uint32_t first_line;
    uint16_t width, height;
    char *data = NULL;
    
    assert (fread (&first_line, 1, 4, fp) == 4);
    assert (fread (&width, 1, 2, fp) == 2);
    assert (fread (&height, 1, 2, fp) == 2);
    
    data = malloc (width * height * 2);
    assert (data);
    
    long mark = ftell (fp);
    fseek (fp, first_line, SEEK_SET);
    assert (fread (data, 1, width * height * 2, fp) == width * height * 2);
    fseek (fp, mark, SEEK_SET);
    
    sprite = c16_sprite_make (width, height, (uint16_t*)data);
    c16_set_sprite (image, i, sprite);
  }
  
  fclose (fp);
  
  return image;
}

blk_t*
blk_new (c16_format_t format, uint16_t width, uint16_t height, uint16_t *data)
{
  blk_t *blk = malloc (sizeof (blk_t)); {
    blk->format = format;
    blk->width = width;
    blk->height = height;
    blk->data = data;
  }
  return blk;
}

blk_t*
blk_new_from_file (FILE *fp)
{
  blk_t *blk = NULL;
  
  uint32_t header;
  bool is_565;
  uint16_t bgwidth, bgheight, count;
  char* data;
  int i;
  
  assert (fread (&header, 1, 4, fp) == 4);
  
  is_565 = header & 0x1;
  assert (header ^ 0x2); // Is an S16 File; not c16_t
  
  assert (fread (&bgwidth, 1, 2, fp) == 2);
  assert (fread (&bgheight, 1, 2, fp) == 2);
  assert (fread (&count, 1, 2, fp) == 2);
  
  uint32_t offsets[count];
  for (i = 0; i < count; ++i) {
    uint32_t off;
    assert (fread (&off, 1, 4, fp) == 4);
    offsets[i] = off + 4; // Because of the extra width and height in header
    
    uint16_t width, height;
    assert (fread (&width, 1, 2, fp) == 2);
    assert (fread (&height, 1, 2, fp) == 2);
    assert (width == 128);
    assert (height == 128);
  }
  
  data = malloc (sizeof (uint16_t) * bgwidth * 128 * bgheight * 128);
  /*
    So sprites are arranged top-to-bottom, left-to-right. That is, sprite 1 is
    at (0,0) and sprite 2 is at (0, 1). Complicated? Yes
  */
  for (i = 0; i < count; ++i) { // For each sprite
    long mark = ftell (fp);
    fseek (fp, offsets[i], SEEK_SET);
    
    for (int j = 0; j < 128; ++j) // For each pixel-row
      assert (fread (data + j * bgwidth * 256 + (i % bgheight) * 128 * bgwidth * 256 + (i / bgheight) * 256, 1, 256, fp) == 256);
    
    fseek (fp, mark, SEEK_SET);
  }
  
  blk = blk_new (is_565?C16_565:C16_555, bgwidth*128, bgheight*128, (uint16_t*)data);
  return blk;
}

c16_format_t
blk_get_format (blk_t *blk)
{
  return blk->format;
}

uint16_t
blk_get_height (blk_t *blk)
{
  return blk->height;
}

uint16_t
blk_get_width (blk_t *blk)
{
  return blk->width;
}

uint16_t*
blk_get_data (blk_t *blk)
{
  return blk->data;
}
