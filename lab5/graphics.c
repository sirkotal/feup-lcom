#include "graphics.h"

#define FAIL 1
#define SUCCESS 0

int (set_graphic_mode)(uint16_t submode) {
    reg86_t reg86;
    memset(&reg86, 0, sizeof(reg86)); // initialize all values at 0
    reg86.intno = 0x10;               // intno is always 0x10      
    reg86.ah = 0x4F;                  // AX MSB
    reg86.al = 0x02;                  // AX LSB (graphic mode is 0x02)

    reg86.bx = submode | BIT(14);     // linear memory addressing
    if (sys_int86(&reg86)) {     
        printf("Set graphic mode failed\n");
        return FAIL;
    }
    return SUCCESS;
}

int (set_frame_buffer)(uint16_t mode) {
  memset(&mode_info, 0, sizeof(mode_info));   // get mode info
  
  if (vbe_get_mode_info(mode, &mode_info)) {
    return FAIL;
  }

  unsigned int bytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;
  unsigned int frame_size = mode_info.XResolution * mode_info.YResolution * bytesPerPixel;
  
  struct minix_mem_range physic_addresses;
  physic_addresses.mr_base = mode_info.PhysBasePtr;
  physic_addresses.mr_limit = physic_addresses.mr_base + frame_size;
  
  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &physic_addresses)) {
    printf("Failed to allocate physical memory\n");
    return FAIL;
  }

  frame_buffer = vm_map_phys(SELF, (void*) physic_addresses.mr_base, frame_size);
  if (frame_buffer == NULL) {
    printf("Failed to allocate virtual memory\n");
    return FAIL;
  }

  return SUCCESS;
}

int (color_pixel)(uint16_t x, uint16_t y, uint32_t color) {
  if(x > mode_info.XResolution || y > mode_info.YResolution) {
    return FAIL;
  }
  
  unsigned bytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;

  unsigned int index = (mode_info.XResolution * y + x) * bytesPerPixel;

  if (memcpy(&frame_buffer[index], &color, bytesPerPixel) == NULL) {
    return FAIL;
  }

  return SUCCESS;
}

int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (unsigned i = 0 ; i < len ; i++) {
      if (color_pixel(x+i, y, color)) {
        return FAIL;
      }
  }   
  return SUCCESS;
}

int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for(unsigned i = 0; i < height ; i++)
    if (vg_draw_hline(x, y+i, width, color)) {
      vg_exit();
      return FAIL;
    }
  return SUCCESS;
}

uint32_t (R)(uint32_t first) {
  return ((1 << mode_info.RedMaskSize) - 1) & (first >> mode_info.RedFieldPosition);
}

uint32_t (G)(uint32_t first) {
  return ((1 << mode_info.GreenMaskSize) - 1) & (first >> mode_info.GreenFieldPosition);
}

uint32_t (B)(uint32_t first) {
  return ((1 << mode_info.BlueMaskSize) - 1) & (first >> mode_info.BlueFieldPosition);
}

int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  xpm_image_t img;

  uint8_t *colors = xpm_load(xpm, XPM_INDEXED, &img);

  for (int h = 0 ; h < img.height ; h++) {
    for (int w = 0 ; w < img.width ; w++) {
      if (color_pixel(x + w, y + h, *colors)) {
        return FAIL;
      }
      colors++;
    }
  }
  return SUCCESS;
}
