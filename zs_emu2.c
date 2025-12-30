#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define FAST 1

#include <pdcurses.h>
void hex_dump_curses(const char *data, size_t offset) {
    int height, width;
    getmaxyx(stdscr, height, width);
    
    erase();
    for (int line = 0; line < height; line++)
    {
        mvprintw(line, 0, "%08lx  ", offset + line * 16);
        for (int j = 0; j < 16; j++)
            mvprintw(line, 10 + j * 3, "%02x ", (unsigned char)data[offset + line * 16 + j]);
        for (int j = 0; j < 16; j++)
            mvprintw(line, 10 + j + 16*3 + 2, "%c", (unsigned char)data[offset + line * 16 + j]);
    }
    
    refresh();
}

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

char * read_file(const char * path, long * _len)
{
    long len;
    FILE * f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    char * buf = (char *)malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    if (_len) *_len = len;
    return buf;
}

#define mem_int int
#define memsize (1<<24)

void be_slow()
{
    int _slow_down = 100;
    while (_slow_down > 0)
    {
        asm inline("":"+r"(_slow_down));
        _slow_down -= 1;
    }
}

void scancodes_init(char * scodes);

int main(int argc, char ** argv)
{
    assert(argc > 1);
    long len;
    
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, 1);
    keypad(stdscr, 1);
    
    char * memory = (char *)malloc(memsize);
    memset(memory, 0x73, memsize);
    assert(memory);
    
    char * _source = read_file(argv[1], &len);
    memcpy(memory + 0x8008, _source, len);
    char * source = memory + 0x8008;
    int source_int = 0x8008;
    char * source2 = read_file("dumb.txt", 0);
    
    char * shadow_vram = (char *)malloc(8000);
    
    
    for (int y = 0; y < 25; y++)
    {
        for (int x = 0; x < 80; x++)
            memory[0xb8000 + y*160 + x*2 + 1] = 0x0F;
    }
    
    memcpy(shadow_vram, memory+0xb8000, 5000);
    
    // list of pointers to strings making up the file
    int ptr_table    = 2000004;
    // 3000000 is where the freelist lives
    // 3000004 is where the "next item" value lives
    int string_table = 3000008;
    
    int linecount = 0;
    while (*source2)
    {
        linecount++;
        unsigned char len = strcspn(source2, "\n");
        memcpy(memory + ptr_table + 98304, &string_table, 4);
        memory[string_table + 98304] = len;
        memcpy(memory + string_table + 98304 + 1, source2, len);
        //*(memory + string_table + 398304 + 1 + len) = '%';
        
        string_table += 256;
        ptr_table += 4;
        source2 += len;
        if (*source2 == '\n') source2++; // skip newline if it exists
    }
    unsigned int _q = 488444439;
    memcpy(memory + 3000000 + 98304, &_q, 4);
    memcpy(memory + 3000004 + 98304, &string_table, 4);
    
    printf("linecount %d\n", linecount);
    memcpy(memory + 2000000 + 98304, &linecount, 4);
    
    //int macros[1024];
    int * macros = (int *)(memory + 0x10010);
    memset(macros, 0xFE, 1024*4);
    int macros_i = 0;
    
    //int stack[1024];
    int * stack = (int *)(memory + 0x6a00);
    memset(stack - 1024, 0xFE, 1024*4);
    int stack_i = 0;
    
    #define push_int(X) { assert(-stack_i < 1024); stack[--stack_i] = (X); }
    #define pop_int(X) { assert(-stack_i > 0); X = stack[stack_i++]; }
    #define readnum(X) { \
        unsigned int _x = 0; \
        unsigned int c = 0; \
        do { \
            be_slow(); \
            _x *= 10; \
            _x += c; \
            c = source[i++] - '0'; \
        } while (c < 10); \
        /*printf("read num %u\n", _x);*/ \
        X = _x; \
    }
    
    int i = 0;
    
    #if FAST
    int ____labels = 0x8008 + 0x10010*8;
    ____labels = ____labels*8;
    int * _labels = (int *)(memory + ____labels);
    #endif
    
    short _macro_cnt = 0;
    while (source[i] != 0)
    {
        char c = source[i];
        i++;
        if (c == '#')
        {
            macros[macros_i++] = i + source_int;
            assert(macros_i < 1024);
            c = source[i];
            if (c >= '0' && c <= '9')
            {
                int x;
                readnum(x);
                #if FAST
                _labels[x*2] = i;
                #endif
                continue;
            }
        }
    }
    
    i = 0;
    #if FAST
    while (source[i] != 0)
    {
        char c = source[i];
        i++;
        if (c <= 32) continue;
        if (c == '?') continue;
        if (c == ';') continue;
        if (c == '@') { i++; continue; }
        if (c == ',') continue;
        if (c == '#') continue;
        if (c >= '0' && c <= '9') continue;
        // default
        {
            i--;
            int nlp = strcspn(source+i, "\n");
            //printf("---------at %d:%.*s\n", i, nlp, source+i);
            
            int j;
            int k;
            
            int q = 0;
            
            do {
                j = macros[q++] - source_int;
                if (q >= macros_i)
                    break;
                k = i;
                
                while (source[j] > 32 && source[j] == source[k])
                {
                    j++;
                    k++;
                }
            } while (source[j] > 32 || source[k] > 32);
            i++;
            if (q >= macros_i)
                continue;
            _labels[(i-1)*2+1] = j;
            //printf("         picked %u (under %u)\n", j, k);
        }
    }
    i = 0;
    #endif
    
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window * win = SDL_CreateWindow("VGA x SDL2 (high level zerostrap-x86 emulator)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 80*9, 25*16, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    SDL_Surface * font_img = IMG_Load("vga.png");
    SDL_Texture * font_tex = SDL_CreateTextureFromSurface(renderer, font_img);
    SDL_SetTextureAlphaMod(font_tex, 255);
    SDL_FreeSurface(font_img);
    
    // sdl -> ps/2 scancode lookup table
    char scancodes[256];
    memset(scancodes, 0, 256);
    scancodes_init(scancodes);
    
    unsigned char keybuf[64];
    int keybuf_i = 0;
    
    int first_render = 1;
    int vram_hit = 0;
    int dirty = 0;
    
    unsigned int q = SDL_GetTicks();
    unsigned int event_throttle = SDL_GetTicks();
    while (1)
    {
        TOP:
        { }
        be_slow();
        
        SDL_Event event;
        if (SDL_GetTicks() - event_throttle > 5)
        {
            {
                static int  offs = 2000000;
                static int bkmarks[10] = {-98304, 0x6a00 - 98304 - 16*16, 0x8008 - 98304, 0x10010-98304, 0, 2000000, 3000000, 0, 0, 0};
                if (offs < -98304)
                    offs = -98304;
                
                hex_dump_curses(memory, offs + 98304);
                int ch = wgetch(stdscr); 
                if (ch != ERR) {
                    printf("%d %x\n", ch - KEY_OFFSET, ch - KEY_OFFSET);
                    if (ch >= '0' && ch <= '9')
                        offs = bkmarks[ch - '0'];
                    if (ch >= ALT_0 && ch <= ALT_9)
                        bkmarks[ch - ALT_0] = offs;
                    switch (ch) {
                        case KEY_SPREVIOUS:
                            offs -= 16*24*24;
                            break;
                        case KEY_SNEXT:
                            offs += 16*24*24;
                            break;
                        case KEY_PPAGE:
                            offs -= 16*24;
                            break;
                        case KEY_NPAGE:
                            offs += 16*24;
                            break;
                        case KEY_UP:
                            offs -= 16;
                            break;
                        case KEY_DOWN:
                            offs += 16;
                            break;
                    }
                    wrefresh(stdscr);
                }
            }
            
            event_throttle = SDL_GetTicks();
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_KEYDOWN: 
                case SDL_KEYUP: {
                    if (keybuf_i > 4) continue;
                    if (event.key.keysym.scancode >= 256) continue;
                    if (event.key.keysym.scancode == SDL_SCANCODE_PRINTSCREEN) continue; // TODO
                    if (event.key.keysym.scancode == SDL_SCANCODE_PAUSE) continue; // TODO
                    //printf("%u\n", event.key.keysym.scancode);
                    unsigned char k = scancodes[event.key.keysym.scancode];
                    if (k == 0) continue;
                    if (k >= 0x80)
                    {
                        keybuf[keybuf_i++] = 0xE0;
                        k &= 0x7F;
                    }
                    if (event.type == SDL_KEYUP)
                        k |= 0x80;
                    
                    keybuf[keybuf_i++] = k;
                } break;
                case SDL_QUIT: {
                    exit(0);
                } break;
                }
            }
        }
        
        if (SDL_GetTicks() - q > 5)
        {
            q = SDL_GetTicks();
            if (vram_hit)
            {
                //printf("---------%d %d %d\n", i, macros_i, stack_i);
                vram_hit -= 1;
                //printf("???");
                //printf("\033[0;0H");
                //printf("\033[2J");
                int start_y = vram_hit / 160;
                int start_x = (vram_hit % 160) / 2;
                //if (first_render) start_x = start_y = 0;
                start_x = start_y = 0;
                int w = 0;
                for (int y = start_y; y < 25; y++)
                {
                    for (int x = start_x; x < 80; x++)
                    {
                        start_x = 0;
                        
                        unsigned char att = memory[0xb8000 + y*160 + x*2 + 1];
                        unsigned char c = memory[0xb8000 + y*160 + x*2 + 0];
                        unsigned char prev_att = shadow_vram[y*160 + x*2 + 1];
                        unsigned char prev_c = shadow_vram[y*160 + x*2 + 0];
                        if (att == prev_att && c == prev_c)
                            continue;
                        //puts("different. applying.");
                        
                        shadow_vram[y*160 + x*2 + 1] = att;
                        shadow_vram[y*160 + x*2 + 0] = c;
                        
                        unsigned char bg = (att >> 4);
                        unsigned char fg = att & 0xf;
                        
                        SDL_Rect src = {0,0,9,16};
                        SDL_Rect dst = {0,0,9,16};
                        
                        dst.x = x * 9;
                        dst.y = y * 16;
                        
                        static SDL_Color last_bgc = {};
                        SDL_Color bgc;
                        bgc.r = (!!(bg&4))*170 + (!!(bg&8))*85;
                        bgc.g = (!!(bg&2))*170 + (!!(bg&8))*85;
                        bgc.b = (!!(bg&1))*170 + (!!(bg&8))*85;
                        if (bg == 6) bgc.g = 0x55;
                        
                        
                        src.x = (c * 9) % 288;
                        src.y = ((c * 9) / 288) * 16;
                        static SDL_Color last_fgc = {};
                        SDL_Color fgc;
                        fgc.r = (!!(fg&4))*170 + (!!(fg&8))*85;
                        fgc.g = (!!(fg&2))*170 + (!!(fg&8))*85;
                        fgc.b = (!!(fg&1))*170 + (!!(fg&8))*85;
                        if (fg == 6) fgc.g = 0x55;
                        
                        SDL_SetRenderDrawColor(renderer, bgc.r, bgc.g, bgc.b, 255);
                        SDL_RenderFillRect(renderer, &dst);
                        SDL_SetTextureColorMod(font_tex, fgc.r, fgc.g, fgc.b);
                        SDL_RenderCopy(renderer, font_tex, &src, &dst);
                        
                        w++;
                        //if (!first_render && w > 3) goto LEAVELOOP;
                    }
                }
                LEAVELOOP:
                { }
                
                //first_render = 0;
                vram_hit = 0;
            }
            
            SDL_RenderPresent(renderer);
        }
        
        //int nlp = strcspn(source+i, "\n");
        //int nlp2 = strcspn(source+i, ";");
        //if (nlp > nlp2) nlp = nlp2;
        //printf("---------at %d:%.*s\n", i, nlp, source+i);
        
        assert(i < len);
        unsigned int c = source[i++];
        if (c <= 32) continue;
        
        if (c == '?')
        {
            unsigned mem_int val;
            unsigned int target;
            pop_int(val);
            pop_int(target);
            //printf("...? %d %d\n", val, target);
            if (val)
            {
                #if FAST
                    i = _labels[target*2];
                    goto TOP;
                #else
                    int j = 0;
                    while (1)
                    {
                        be_slow();
                        //printf("! %d\n", j);
                        //i = macros[j++];
                        i = macros[macros_i - ++j] - source_int;
                        assert(i < (1<<30));
                        assert(j < 1024);
                        
                        unsigned int x;
                        readnum(x);
                        
                        if (x == target)
                            goto TOP;
                    }
                #endif
            }
            
            continue;
        }
        
        if (c == ';')
        {
            macros_i -= 1;
            i = macros[macros_i] - source_int;
            assert(macros_i > 0);
            continue;
        }
        
        if (c == '@')
        {
            c = source[i++];
            mem_int b;
            pop_int(b);
            
            if (c == 'w')
            {
                unsigned mem_int loc = b;
                assert(loc < (memsize));
                mem_int val;
                pop_int(val);
                memcpy(memory + loc, &val, 4);
                
                //if (loc > 98504)
                //    printf("??? %u <- %d\n", loc, val);
                
                if (loc >= 0xb8000 && loc < 0xc0000)
                    vram_hit = loc - 0xb8000 + 1;
            }
            if (c == '-')
            {
                unsigned int a;
                pop_int(a);
                push_int(a-b);
                pop_int(a);
                push_int(a);
            }
            if (c == '&')
            {
                unsigned int a;
                pop_int(a);
                push_int(a&(unsigned int)b);
            }
            if (c == '*')
            {
                unsigned int a;
                pop_int(a);
                push_int(a*b);
            }
            if (c == '\\')
            {
                int a;
                pop_int(a);
                push_int((((int)a)>>((int)b)));
            }
            if (c == 'r')
            {
                unsigned mem_int loc = b;
                assert(loc < (memsize));
                int val;
                memcpy(&val, memory + loc, 4);
                //printf("%u was %u (%u)\n", loc, val, val & 0xFF);
                push_int(val);
            }
            if (c == '/')
            {
                int a;
                pop_int(a);
                push_int(a/b);
            }
            if (c == 'C')
            {
                //printf("asdokaerguoiaherg\n");
                //printf("%zu %zu %zu %zu\n", macros_i, macros[macros_i-1], source, memory);
                uint32_t val = (uint64_t)(macros + (macros_i)) - (uint64_t)(memory);
                push_int(val);
                printf("-> %zu (%X)\n", val, val);
            }
            continue;
        }
        if (c == ',')
        {
            if (keybuf_i == 0)
            {
                static int q = 0;
                q++;
                if (q == 10)
                {
                    q = 0;
                    SDL_Delay(1);
                }
                push_int(0);
            }
            else
            {
                unsigned int k = keybuf[0];
                for (int i = 0; i < keybuf_i; i++)
                    keybuf[i] = keybuf[i+1];
                keybuf_i--;
                //printf("received key %c (%u)\n", (char)k, k);
                push_int(0x100 | k);
            }
            
            continue;
        }
        
        if (c == '#')
        {
            while (source[i] >= 32)
                i++;
            continue;
        }
        
        i--;
        if (c >= '0' && c <= '9')
        {
            unsigned int x;
            readnum(x);
            //printf("pushing %u\n", x);
            i--;
            push_int(x);
            //printf("pushed %u\n", x);
            continue;
        }
        
        // default
        {
            #if FAST
                //int nlp = strcspn(source+i, "\n");
                //printf(".....at %d:%.*s\n", i, nlp, source+i);
                int k = i;
                while (source[i] > 32)
                    i++;
                macros[macros_i++] = i + source_int;
                i = _labels[k*2+1];
                //printf("going to %u\n", i);
                assert((int)i > 0);
            #else
                int j;
                int k;
                
                int q = 0;
                
                do {
                    be_slow();
                    //j = macros[q++];
                    j = macros[macros_i - ++q] - source_int;
                    if (q > macros_i)
                        printf("-----------%s\n", source+i);
                    assert(q <= macros_i);
                    k = i;
                    
                    while (source[j] > 32 && source[j] == source[k])
                    {
                        be_slow();
                        j++;
                        k++;
                    }
                } while (source[j] > 32 || source[k] > 32);
                
                i = j;
                
                assert(macros_i < 1024);
                macros[macros_i++] = k - source_int;
            #endif
        }
    }
    
    return 0;
}

void scancodes_init(char * scodes)
{
    scodes[SDL_SCANCODE_ESCAPE] = 0x01;
    scodes[SDL_SCANCODE_1] = 0x02;
    scodes[SDL_SCANCODE_2] = 0x03;
    scodes[SDL_SCANCODE_3] = 0x04;
    scodes[SDL_SCANCODE_4] = 0x05;
    scodes[SDL_SCANCODE_5] = 0x06;
    scodes[SDL_SCANCODE_6] = 0x07;
    scodes[SDL_SCANCODE_7] = 0x08;
    scodes[SDL_SCANCODE_8] = 0x09;
    scodes[SDL_SCANCODE_9] = 0x0A;
    scodes[SDL_SCANCODE_0] = 0x0B;
    scodes[SDL_SCANCODE_MINUS] = 0x0C;
    scodes[SDL_SCANCODE_EQUALS] = 0x0D;
    scodes[SDL_SCANCODE_BACKSPACE] = 0x0E;
    scodes[SDL_SCANCODE_TAB] = 0x0F;
    scodes[SDL_SCANCODE_Q] = 0x10;
    scodes[SDL_SCANCODE_W] = 0x11;
    scodes[SDL_SCANCODE_E] = 0x12;
    scodes[SDL_SCANCODE_R] = 0x13;
    scodes[SDL_SCANCODE_T] = 0x14;
    scodes[SDL_SCANCODE_Y] = 0x15;
    scodes[SDL_SCANCODE_U] = 0x16;
    scodes[SDL_SCANCODE_I] = 0x17;
    scodes[SDL_SCANCODE_O] = 0x18;
    scodes[SDL_SCANCODE_P] = 0x19;
    scodes[SDL_SCANCODE_LEFTBRACKET] = 0x1A;
    scodes[SDL_SCANCODE_RIGHTBRACKET] = 0x1B;
    scodes[SDL_SCANCODE_RETURN] = 0x1C;
    scodes[SDL_SCANCODE_LCTRL] = 0x1D;
    scodes[SDL_SCANCODE_A] = 0x1E;
    scodes[SDL_SCANCODE_S] = 0x1F;
    scodes[SDL_SCANCODE_D] = 0x20;
    scodes[SDL_SCANCODE_F] = 0x21;
    scodes[SDL_SCANCODE_G] = 0x22;
    scodes[SDL_SCANCODE_H] = 0x23;
    scodes[SDL_SCANCODE_J] = 0x24;
    scodes[SDL_SCANCODE_K] = 0x25;
    scodes[SDL_SCANCODE_L] = 0x26;
    scodes[SDL_SCANCODE_SEMICOLON] = 0x27;
    scodes[SDL_SCANCODE_APOSTROPHE] = 0x28;
    scodes[SDL_SCANCODE_GRAVE] = 0x29;
    scodes[SDL_SCANCODE_LSHIFT] = 0x2A;
    scodes[SDL_SCANCODE_BACKSLASH] = 0x2B;
    scodes[SDL_SCANCODE_Z] = 0x2C;
    scodes[SDL_SCANCODE_X] = 0x2D;
    scodes[SDL_SCANCODE_C] = 0x2E;
    scodes[SDL_SCANCODE_V] = 0x2F;
    scodes[SDL_SCANCODE_B] = 0x30;
    scodes[SDL_SCANCODE_N] = 0x31;
    scodes[SDL_SCANCODE_M] = 0x32;
    scodes[SDL_SCANCODE_COMMA] = 0x33;
    scodes[SDL_SCANCODE_PERIOD] = 0x34;
    scodes[SDL_SCANCODE_SLASH] = 0x35;

    scodes[SDL_SCANCODE_RSHIFT] = 0x36;
    scodes[SDL_SCANCODE_KP_MULTIPLY] = 0x37;
    scodes[SDL_SCANCODE_LALT] = 0x38;
    scodes[SDL_SCANCODE_SPACE] = 0x39;
    scodes[SDL_SCANCODE_CAPSLOCK] = 0x3A;
    scodes[SDL_SCANCODE_F1] = 0x3B;
    scodes[SDL_SCANCODE_F2] = 0x3C;
    scodes[SDL_SCANCODE_F3] = 0x3D;
    scodes[SDL_SCANCODE_F4] = 0x3E;
    scodes[SDL_SCANCODE_F5] = 0x3F;
    scodes[SDL_SCANCODE_F6] = 0x40;
    scodes[SDL_SCANCODE_F7] = 0x41;
    scodes[SDL_SCANCODE_F8] = 0x42;
    scodes[SDL_SCANCODE_F9] = 0x43;
    scodes[SDL_SCANCODE_F10] = 0x44;
    scodes[SDL_SCANCODE_NUMLOCKCLEAR] = 0x45;
    scodes[SDL_SCANCODE_SCROLLLOCK] = 0x46;
    scodes[SDL_SCANCODE_KP_7] = 0x47;
    scodes[SDL_SCANCODE_KP_8] = 0x48;
    scodes[SDL_SCANCODE_KP_9] = 0x49;
    scodes[SDL_SCANCODE_KP_MINUS] = 0x4A;
    scodes[SDL_SCANCODE_KP_4] = 0x4B;
    scodes[SDL_SCANCODE_KP_5] = 0x4C;
    scodes[SDL_SCANCODE_KP_6] = 0x4D;
    scodes[SDL_SCANCODE_KP_PLUS] = 0x4E;
    scodes[SDL_SCANCODE_KP_1] = 0x4F;
    scodes[SDL_SCANCODE_KP_2] = 0x50;
    scodes[SDL_SCANCODE_KP_3] = 0x51;
    scodes[SDL_SCANCODE_KP_0] = 0x52;
    scodes[SDL_SCANCODE_KP_PERIOD] = 0x53;
    scodes[SDL_SCANCODE_F11] = 0x57;
    scodes[SDL_SCANCODE_F12] = 0x58;
    
    // multibyte items (0xE0, x & 0x7F)
    
    scodes[SDL_SCANCODE_KP_ENTER] = 0x9C;
    scodes[SDL_SCANCODE_RCTRL] = 0x9D;
    scodes[SDL_SCANCODE_KP_DIVIDE] = 0xB5;
    scodes[SDL_SCANCODE_RALT] = 0xB8;
    scodes[SDL_SCANCODE_LEFT] = 0xCB;
    scodes[SDL_SCANCODE_HOME] = 0xC7;
    scodes[SDL_SCANCODE_UP] = 0xC8;
    scodes[SDL_SCANCODE_PAGEUP] = 0xC9;
    scodes[SDL_SCANCODE_RIGHT] = 0xCD;
    scodes[SDL_SCANCODE_END] = 0xCF;
    scodes[SDL_SCANCODE_DOWN] = 0xD0;
    scodes[SDL_SCANCODE_PAGEDOWN] = 0xD1;
    scodes[SDL_SCANCODE_INSERT] = 0xD2;
    scodes[SDL_SCANCODE_DELETE] = 0xD3;
    scodes[SDL_SCANCODE_LGUI] = 0xDB;
    scodes[SDL_SCANCODE_RGUI] = 0xDC;
    scodes[SDL_SCANCODE_APPLICATION] = 0xDD;
}