
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_GREEN 0xFF22EE22
#define COLOR_BLUE 0xFF2222FF
#define COLOR_RED 0xFFEE3333

typedef struct rect_t
{
    int x;
    int y;
    int sx;
    int sy;
}
rect_t;

uint32_t* buffer = 0;
uint32_t* window_buffer = 0;

void draw_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || x >= FRAME_SX || y >= FRAME_SY) {
        return;
    }

    uint32_t* dest_p = &buffer[FRAME_SX*y + x];

    uint8_t src_b = (uint8_t) color;
    uint8_t src_g = (uint8_t) (color >> 8);
    uint8_t src_r = (uint8_t) (color >> 16);
    uint8_t src_a = (uint8_t) (color >> 24);

    uint32_t dest = *dest_p;
    uint8_t dest_b = (uint8_t) dest;
    uint8_t dest_g = (uint8_t) (dest >> 8);
    uint8_t dest_r = (uint8_t) (dest >> 16);
    uint8_t dest_a = (uint8_t) (dest >> 24);

    float src_fac = src_a/255.f;
    float dest_fac = 1-(src_a/255.f);

    dest_r = src_r * src_fac + dest_r * dest_fac;
    dest_g = src_g * src_fac + dest_g * dest_fac;
    dest_b = src_b * src_fac + dest_b * dest_fac;
    dest_a = 255;

    dest = 0;
    dest = dest_b | ((uint32_t) dest_g << 8) | ((uint32_t) dest_r << 16) | ((uint32_t) dest_a << 24); 

    *dest_p = dest;
}

void draw_horizontal_line(int x_start, int x_end, int y)
{
    for (int x = x_start; x < x_end; x++) {
        draw_pixel(x, y, 0xffffffff);
    }
}

void draw_vertical_line(int x, int y_start, int y_end)
{
    for (int y = y_start; y < y_end; y++) {
        draw_pixel(x, y, 0xffffffff);
    }
}

void draw_box(rect_t r, uint32_t color)
{
    for (int _y = r.y; _y < r.y + r.sy; _y++) {
        for (int _x = r.x; _x < r.x + r.sx; _x++) {
            draw_pixel(_x, _y, color);
        }
    }
}

void draw_border(rect_t r, uint32_t color)
{
    draw_horizontal_line(r.x, r.x + r.sx - 1, r.y);
    draw_horizontal_line(r.x, r.x + r.sx - 1, r.y + r.sy - 1);
    draw_vertical_line(r.x, r.y, r.y + r.sy - 1);
    draw_vertical_line(r.x + r.sx - 1, r.y, r.y + r.sy - 0); // no - 1 is intentional, to fill bottom right pixel
}

// Fonction qui rescale un (frame) buffer source dans buffer destination.
// Utile pour faire un jeu retro: vous faites le rendu dans un buffer du
// style 160×192, puis le copiez dans le framebuffer de la window. Il faut
// que la window soit un nombre entier de fois plus grande que le buffer
// source (genre 3x ou 4x).
void resize_bitmap(uint32_t* dest, int dest_sx, int dest_sy, uint32_t* src, int src_sx, int src_sy)
{
    for (int y = 0; y < dest_sy; y++) {
        for (int x = 0; x < dest_sx; x++) {
            int src_x = x * src_sx / dest_sx;
            int src_y = y * src_sy / dest_sy;
            dest[y*dest_sx + x] = src[src_y*src_sx + src_x];
        }
    }
}

typedef struct bitmap_t
{   
    int size_px_x;
    int size_px_y;
    uint32_t* pixels;
}
bitmap_t;

bitmap_t load_bitmap(const char* filename)
{

    bitmap_t bitmap = {};
    int channel = 0;
    bitmap.pixels = (uint32_t*) stbi_load(filename, &bitmap.size_px_x, &bitmap.size_px_y, &channel, 4);
    if (!bitmap.pixels) {
        printf("Couldn't load bitmap from file '%s'.\n", filename);
        exit(-1);
    }
    for (int i = 0; i < bitmap.size_px_x * bitmap.size_px_y; i++) {
        uint32_t copy = bitmap.pixels[i];
        uint8_t* src_px = (uint8_t*) &copy;
        uint8_t* dest_px = (uint8_t*) &bitmap.pixels[i];
        dest_px[2] = src_px[0]; // R
        dest_px[1] = src_px[1]; // G
        dest_px[0] = src_px[2]; // B
        dest_px[3] = src_px[3]; // A
    }
    return bitmap;
}

uint32_t tint_color(uint32_t col, uint32_t tint)
{
    uint8_t* comp = (uint8_t*) &col;
    uint8_t* tint_comp = (uint8_t*) &tint;
    comp[0] = MIN(comp[0] * (tint_comp[0] / 255.f), 255);
    comp[1] = MIN(comp[1] * (tint_comp[1] / 255.f), 255);
    comp[2] = MIN(comp[2] * (tint_comp[2] / 255.f), 255);
    comp[3] = MIN(comp[3] * (tint_comp[3] / 255.f), 255);
    return col;
}

void draw_bitmap(int posx, int posy, bitmap_t bm, uint32_t tint_color = 0)
{
    for (int y = 0; y < bm.size_px_y; y++) {
        for (int x = 0; x < bm.size_px_x; x++) {
            uint32_t pixel_col = bm.pixels[y * bm.size_px_x + x];
            uint8_t* comp = (uint8_t*) &pixel_col;
            uint8_t* tint_comp = (uint8_t*) &tint_color;
            comp[0] = MIN(comp[0] + tint_comp[0], 255);
            comp[1] = MIN(comp[1] + tint_comp[1], 255);
            comp[2] = MIN(comp[2] + tint_comp[2], 255);
            draw_pixel(posx + x, posy + y, pixel_col);
        }
    }
}

bitmap_t font_map;



#define WOOD_ICN_STR "\x02"
#define FOOD_ICN_STR "\x03"
#define STONE_ICN_STR "\x04"
#define GOLD_ICN_STR "\x05"

void draw_text(int x, int y, const char* text, uint32_t color = 0xFFFFFFFF)
{
    assert(font_map.pixels && "Font map must be loaded");
    
    for (int i = 0; ; i++) {
        char c = text[i];
        if (c == 0) {
            break;
        }
        int row = -1;
        int col = -1;
        if (c >=  'A' && c <= 'Z') {
            row = 0;
            col = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            row = 1;
            col = c - 'a';
        } else if (c >= '!' && c <= '?') {
            row = 2;
            col = c - '!';
        } else if (c == ' ') {
            x += 7;
            continue;
        } else if (c >= 1 && c <= 5) {
            row = 3;
            col = c - 1;
        }
        else {
            // draw a question mark if the character is not present in the map
            x += 7;
            row = 2;
            col = '?' - '!';
        }
        int cx = col * 7;
        int cy = row * 11;
        int size_x = 0;
        for (int by = 0; by < 11; by++) {
            for (int bx = 0; bx < 7; bx++) {
                uint32_t px_color = font_map.pixels[(cy + by) * font_map.size_px_x + (cx + bx)];
                px_color = tint_color(px_color, color);
                draw_pixel(x + bx, y + by, px_color);
                if (size_x < bx && ((px_color >> 24) & 0xff) > 0) {
                    size_x = bx;
                }
            }
        }
        x += size_x + 1;
    }
}
