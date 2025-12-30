# ----
# language bootstrap macros
# ----

#r 0 98304@-@-@r;
#w 0 98304@-@-@w;
#- @-;
#& @&;
#, @,;
#/ @/;
#* @*;
#dup 9000@w 9000@r 9000@r;
#dup2 9004@w 9000@w 9000@r 9004@r 9000@r 9004@r;
#SW 9000@w 9004@w 9000@r 9004@r;
#~ 0 1 - SW - ;
#neg 0 SW - ;
#+ neg - ;
#| ~ SW ~ & ~ ;
#^ dup2 | 9010@w & ~ 9010@r & ;

#rem 9014@w 9010@w 9010@r 9014@r / 9014@r * 9010@r SW - ;
#% rem 9014@r + 9014@r rem ;

# *(int*)0 = INT_MIN
2048 2048 512 * * 9100@w

#ltz 30SW 9100@r@&? 32 1?#30 1;#32 0;
#_not 1 SW - ;
# #< 9044@w 9040@w 1 9044@r ltz 9040@r ltz - 9040@r 9044@r - ltz - - 2 / ;
#< - ltz ;
#>= < _not ;
#> SW < ;
#<= SW >= ;
#== dup2 >= 9050@w <= 9050@r & ;
#!= == _not ;

#w8 9160@w 9164@w 9160@r 1+ r 9165@w 9164@r 9160@r w ;
#r8 r 255 & ;

0 100w
7 .COLOR<-
4 1? # skip over definitions
#.
    655360 100r + w8
    # 7: light grey. 15: white. 8: dark grey. others: various.
    #.COLOR 40r ;
    #.COLOR<- 40w ;
    .COLOR 655361 100r + w8
    100r 2 + 100w
;
#memcpy4
    4w 84w 80w
    33 4r 0 == ?
    #31
    84r r 80r w   84r 4 + 84w   80r 4 + 80w   4r 1 - 4w   31 4r ?
    #33
;
#memcpy
    4w 84w 80w
    43 4r 0 == ?
    #41
    84r r 80r w8   84r 1 + 84w   80r 1 + 80w   4r 1 - 4w   41 4r ?
    #43
;
#memcpy_rev
    4w 4r + 84w 4r +  80w
    53 4r 0 == ?
    #51
    84r 1 - 84w   80r 1 - 80w   84r r 80r w8   4r 1 - 4w   51 4r ?
    #53
;
#memmove
    4w 80w 84w
    64 84r 80r < ?
        84r 80r 4r memcpy_rev
    65 1?
    #64
        84r 80r 4r memcpy
    #65
;
#4
#.cptr 100r 655360 + ;
#.c 100r 2 / ;
#.c<- 2 * 100w ;
#.LF 100r 160+ 160/ 160* 100w ;

0 655360 3840 + w

# ----
# set up ps/2 -> ascii conversion stuff
# ----

# Helpers to make the encoding table easier to type
#si SW 256 * + ;
#v 2000r ;
#v<- 2000w ;
#vinc v 1 + v<- ;
#vw v w vinc ;
#N si 2000r w 2000r 2 + 2000w ;
#S 2100 + 2000w N ;

2100 2000w

#400 # zero out this memory
0 vw
400 2000r 512 2100 + 1 - < ? # if 2000r < 2100 + 512: goto 400

2100 2000w

# ----
# Actual encoded PS/2 -> ASCII conversion table (with extensions for arrow keys etc)
# ----
 27 27 2 S  33 49 N   64 50 N   35  51 N
 36 52 N    37 53 N   94 54 N   38  55 N
 42 56 N    40 57 N   41 48 N   95  45 N
 43 61 N     8  8 N    9  9 N   81 113 N

 87 119 N     69 101 N   82 114 N   84 116 N
 89 121 N     85 117 N   73 105 N   79 111 N
 80 112 N    123  91 N  125  93 N   10  10 N

 65 97 60 S   83 115 N   68 100 N   70 102 N
 71 103 N     72 104 N   74 106 N   75 107 N
 76 108 N     58  59 N   34  39 N  126  96 N

124  92 86 S  90 122 N   88 120 N   67  99 N
 86 118 N     66  98 N   78 110 N   77 109 N
 60  44 N     62  46 N   63  47 N

 42  42 110 S   32 32 114 S  132 55 142 S
128  56 N      134 57 N       45  45 N     129  52 N
  0  53 N      131 54 N       43  43 N     133  49 N
130  50 N      135 51 N      136  48 N     127  46 N

 10  10 312 S   47  47 362 S  132 132 398 S
128 128 N      134 134 N

129 129 406 S  130 130 410 S  133 133 414 S
131 131 N      135 135 N      136 136 N      127 127 N

#shift 60r ;
#shift<- 60w ;
0 shift<-
#mod 68r ;
#mod<- 68w ;
0 mod<-

# ----
# function activation record stack
# ----
3004 300w
#su 1+ 4* 300r SW + 300w ; stack up
#sd 1+ 4* 300r SW - 300w ; stack down
#sr 1+ 4* 300r SW - r ;
#sw 1+ 4* 300r SW - w ;

4 su

#x   0sr ;
#x<- 0sw ;
#y   1sr ;
#y<- 1sw ;
#z   2sr ;
#z<- 2sw ;
#q   3sr ;
#q<- 3sw ;
#x2   4sr ;
#x2<- 4sw ;
#y2   5sr ;
#y2<- 5sw ;
#z2   6sr ;
#z2<- 6sw ;
#q2   7sr ;
#q2<- 7sw ;

#row   500r ;
#row<- 500w ;
#col   504r ;
#col<- 504w ;
#prevrow   508r ;
#prevrow<- 508w ;

#lines 30r ;
2000004 30w

# 0x1D1D1217, "i did init" == 488444439
511 freelist 488444439 == ?
    # init was not done. need to do it manually.
    1 linecount<-
    3000008 lines w
    3000108 3000004w
    9 3000008w
#511

0 3000000w
#linecount 2000000r ;
#linecount<- 2000000w ;
#freelist 3000000r ;
#freelist<- 3000000w ;

# 3000000: lines freelist
# 3000004: next unallocated line

# #log 5100 ;
# #log<- log 1 + log 11 memmove log w8 ;

# 3000008 2000004w
# 3000200 2000008w
# 3000400 2000012w
# 70 2000004r w8
# 14 2000008r w8
# 61 2000012r w8

# skip definition of various helper functions
401 1?
# shift is 54 or 42. caps lock is 58 (not handled)
#getch
    4 su
    #500
        , x<-
    500  x 256 & 0 == ? # not ready yet
    x 255 & x<-
    # x log<-
    503  x 224 != ? # input modifier 0xE0. note, then wait for next.
        mod 1 + mod<-
        500 1?
    #503
    501  x 54 !=  x 42 !=  & ? # shift press
        1 shift<-
        500 1?
    #501
    502  x 128- 54 !=  x 128- 42 !=  & ? # shift release
        0 shift<-
        500 1?
    #502
    505  x 128 < ? # other release
        0 mod<-
        500 1?
    #505
    504  mod 0 == ? # 0xE0-modified input
        x 128 + x<-
        mod 1 - mod<-
    #504
    x 255 & 2 * shift + 2100 + r 255 &
    4 sd
;
#oneline 90r ;
#oneline<- 90w ;
#refresh
    16 su
    2000 .c<-
    row 3 + 10 / 10 * 10 - y<-
    
    811 row 0 > ?
        0 row<-
    #811
    
    812 row linecount < ?
        linecount 1 - row<-
    #812
    
    810 y 0 > ?
        0 y<-
    #810
    
    # store original y value in x2
    row y - x2<-
    
    y 24 + y2<-
    809 y prevrow != ?
        80 row y - 1 - * 2000 + .c<-
        row 1 - y<-
        y 3 + y2<-
        
        815 oneline 0== ?
            y 1 + y<-
            y2 1 - y2<-
            .c 80 + .c<-
        #815
        
        816 y 0 >= ?
            2000 .c<-
            0 y<-
        #816
        
        808 1?
    #809
    y prevrow<-
    #808
    0 oneline<-
    
    
    #800
        25 z2<-
        805 y row != ?
            63 z2<-
            # clamp cursor position to line
            832 col 73 < ?
                73 col<-
            #832
            830 col curlen < ?
                curlen col<-
            #830
            831 col 0 > ?
                0 col<-
            #831
        #805
        
        0 q2<-
        #807
            0 .cptr q2 + w
            q2 4 + q2<-
        807 q2 160 < ?
        
        803 y linecount >= ?
        
        y 4 * lines + r r8 z<-
        820 z 75 < ?
            75 z<-
        #820
        
        # line number
        y 1 + y<-
        y 1000/ 10% 48+ .
        z2 .cptr 1- w8
        y 100/ 10% 48+ .
        z2 .cptr 1- w8
        y 10/ 10% 48+ .
        z2 .cptr 1- w8
        y 10% 48+ .
        z2 .cptr 1- w8
        y 1 - y<-
        
        #lx 8sr ;
        #lx<- 8sw ;
        
        #ly 9sr ;
        y 4 * lines + r 9sw
        
        0 lx<-
        #802
            lx 1 + lx<-
            32 q<-
            804 lx z > ?
                ly lx + r8 q<-
            #804
            q .
            821 y row !=  lx col 1 + !=  | ?
                4 16 * 14 + .cptr 1 - w
            #821
        802 lx z < lx 73 < & ?
        
        822 y row !=  lx col !=  | ?
            5 16 * 15 + .cptr 1 + w
        #822
        
        822 y row !=  lx col !=  |   lx 73 >= | ?
            2 16 * 15 + .cptr 1 + w
        #822
        
        #803
        
        .LF
        y 1 + y<-
    800 y y2 < ?
    
    1920 2000 + .c<-
    
    #.p . ;
    
    8 .COLOR<-
    print_rest_of_line # Row
    32 .
    row 1 + 10000/ 10% 48+ .p
    row 1 + 1000/ 10% 48+ .p
    row 1 + 100/ 10% 48+ .p
    row 1 + 10/ 10% 48+ .p
    row 1 + 1/ 10% 48+ .p
    print_rest_of_line #   Col
    32 .
    col 1 + 100/ 10% 48+ .p
    col 1 + 10/ 10% 48+ .p
    col 1 + 1/ 10% 48+ .p
    print_rest_of_line #   Len
    32 .
    curlen 100/ 10% 48+ .p
    curlen 10/ 10% 48+ .p
    curlen 1/ 10% 48+ .p
    print_rest_of_line #   Log:
    32 .
    7 .COLOR<-
    
    # 0 x<-
    # #841
    #     32 .
    #     log x + r8 100/ 10% 48+ .p
    #     log x + r8 10/ 10% 48+ .p
    #     log x + r8 1/ 10% 48+ .p
    #     x 1+ x<-
    # 841 x 12 < ?
    
    0 x<-
    #824
        32 .   x 1+ x<-
    824 x 52 < ?
    
    0 .c<-
    .c 2 * 655360 +   .c 2000 + 2 *  655360 +   1000 memcpy4
    
    x2 80 * col + 4 + .c<-
    
    16 sd
;
#curlineptr
    row 4 * lines +
;
#curlen
    curlineptr r r8
;
#curlen<-
    curlineptr r w8
;
#curcharptr
    curlineptr r col + 1 +
;
#curchar
    curlineptr r col + 1 + r8
;
#curchar<-
    curlineptr r col + 1 + w8
;
#alloc_string
    4 su
    # pointer to bottom of bump allocator
    540 freelist ? 
        # if no freelist item...
        3000004r x<-
        # next string is 250 bytes later
        3000004r 256+ 3000004w
        541 1?
    #540
        # freelist item!
        freelist x<-
        x r freelist<-
        0 x w
    #541
    # set string length to 0
    0 x w8
    # return pointer
    x
    4 sd
;
#free_string
    4 su
    x<-
    freelist x w # set *(int *)x to current freelist
    x freelist<- # set freelist to x
    4 sd
;
#callstk_top 0@C 4 - ;
#print_rest_of_line
    4 su
    callstk_top 4 - @r 3 + 98304 - x<-
    
    #510
    x r8 .
    x 1 + x<-
    510 x r8 32 >= ?
    
    4 sd
;
#goto
    4 su
    callstk_top x<- x @r 19 + x 4 - @w ;
    4 sd 1?
    # NOTE: the above read of "x @r" gives a callstack value
    #  that points at at the end of the invokation of the "x" macro.
#401

0 row<-
0 col<-
1 neg prevrow<-

refresh

#600
    getch y<-
    
    # enter
    601 y 10 != ?
        curlineptr 4 +   curlineptr   linecount row - 4 *  memmove
        linecount 1 +   linecount<-
        alloc_string curlineptr w
        
        # split line:
        # copy left side of old into new
        curlineptr r 1 +   curlineptr 4 + r 1 +   col   memmove
        # set new len to left side len
        col   curlineptr r   w8
        # move old right-side contents leftwards
        curlineptr 4 + r 1 +   curlineptr 4 + r 1 + col +   curlineptr 4 + r r8 col -   memmove
        # set old len to right side len
        curlineptr 4 + r r8 col -   curlineptr 4 + r w8
        
        1 neg prevrow<-
        row 1 + row<-
        
        0 col<-
        refresh
        620 1?
    #601
    
    # arrow keys
    602 y 128 != ?
        row 1 - row<-
        refresh
        620 1?
    #602
    603 y 129 != ?
        col 1 - col<-
        1 oneline<-
        refresh
        620 1?
    #603
    604 y 130 != ?
        col 1 + col<-
        1 oneline<-
        refresh
        620 1?
    #604
    605 y 131 != ?
        row 1 + row<-
        refresh
        620 1?
    #605
    
    # backspace
    606 y 8 != ?
        609 col 0 != ?
            # try to splice current line onto previous line (unless line is first line)
            621 row 0 != ?
                4 .COLOR<-
                1954 .c<-
                print_rest_of_line # can't delete line 1
                7 .COLOR<-
                620 1?
            #621
            
            curlineptr r free_string
            curlineptr   curlineptr 4 +   linecount row - 4 *  memmove
            linecount 1 -   linecount<-
            1 neg prevrow<-
            row 1 - row<-
            curlineptr r r8 col<-
            refresh
            620 1?
        #609
        608 curlen 0 != ?
            # delete line
            curlineptr r free_string
            curlineptr   curlineptr 4 +   linecount row - 4 *  memmove
            linecount 1 -   linecount<-
            1 neg prevrow<-
            row 1 - row<-
            curlineptr r r8 col<-
            refresh
            620 1?
        #608
            # delete single character
            col 1 - col<-
            666 col 0 >= ?
                0 col<-
                620 1?
            #666
            curcharptr  curcharptr 1 +  curlen col -  memmove
            curlen 1 - curlen<-
            
            1 oneline<-
            refresh
            
        620 1?
    #606
    
    # col/curlen clamping
    col 1 + col<-
    667 col 73 < ?
        73 col<-
    #667
    curlen 1 + curlen<-
    668 curlen 73 < ?
        73 curlen<-
    #668
    
    curcharptr curcharptr 1 -  curlen col -  memmove
    y curcharptr 1 - w8
    
    1 oneline<-
    refresh
    
    #620
600 1?
終