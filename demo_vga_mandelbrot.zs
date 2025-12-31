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
#401

100 su

#px   100@r;
#px<- 100@w;
#py   104@r;
#py<- 104@w;
#cr   110@r;
#cr<- 110@w;
#ci   114@r;
#ci<- 114@w;
#zr   120@r;
#zr<- 120@w;
#zi   124@r;
#zi<- 124@w;
#i   134@r;
#i<- 134@w;

# Mandelbrot Set Generator

#zoom 200@r;
#zoom<- 200@w;

0 zoom<-

#99
  0 py<-
  #110
    0 px<-
    #120
      px 3000 * 320 / 20 zoom - * 20 / 2000 - cr<-
      py 10 * 1000 - 20 zoom - * 20 / ci<-

      0 zr<-
      0 zi<-
      32 i<-

      #130
        140    zr zr * zi zi * + 4000000 >    i 0 ==    |  ?

        zr zr * 1000 / zi zi * 1000 / - cr +
        2 zr * zi * 1000 / ci + zi<-
        zr<-

        i 1 - i<-
      130 1?

      #140
        145 i 0 == ?
        32 i - 15 +
        146 1?
      #145
        0
      #146
        557056 py 320 * + px +
      w8

      px 1 + px<-
    120 px 320 < ?
    py 1 + py<-
  110 py 200 < ?
  
  zoom 1 + 20 % zoom<-
99 1?

終