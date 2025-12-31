
# ----
# language bootstrap macros
# ----

# Sigil-less versions of primitives.
# The 9000...9999 memory region is reserved for bootstrap macros.
# Reading/writing has a 98304 byte offset to make it hard to accidentally trash the interpreter.

#r 0 98304@-@-@r; # TRANSLATIONM: read(<implicit> - (0 - 98304))
#w 0 98304@-@-@w;
#- @-;
#& @&;
#, @,;
#/ @/;
#* @*;

# "Missing" features.

#dup 9000@w 9000@r 9000@r;
#dup2 9004@w 9000@w 9000@r 9004@r 9000@r 9004@r;
#SW 9000@w 9004@w 9000@r 9004@r; # swap
#+ 9000@w 0 9000@r @-@-;
#rem 9014@w 9010@w 9010@r 9014@r / 9014@r * 9010@r SW - ;
#% rem 9014@r + 9014@r rem ;

# *(int*)9100 = INT_MIN
2147483648 9100@w
# *(int*)9104 = 0xFFFFFF00
4294967040 9104@w


#nez 34SW ?0;#34 1;
#ltz 30SW 9100@r@&?0;#30 1;
#_not 1 SW @- ;
#< @- ltz ;
#>= < _not ;
#> SW < ;
#<= SW >= ;
#== @- nez _not ;
#!= @- nez ;

#w8 9160@w 9164@w 9160@r 0 1 @- @- r 9165@w 9164@r 9160@r w ;
#r8 @r255@&;

#px 100@r;
#px<- 100@w;
#py 104@r;
#py<- 104@w;
#cr 110@r;
#cr<- 110@w;
#ci 114@r;
#ci<- 114@w;
#zr 120@r;
#zr<- 120@w;
#zi 124@r;
#zi<- 124@w;
#i 134@r;
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
        140 zr zr * zi zi * + 4000000 > ?
        145 i 0 == ?
        zr zr * 1000 / zi zi * 1000 / - cr +
        2 zr * zi * 1000 / ci + zi<-
        zr<-
        i 1 - i<-
      130 1?

      #140
        32 i - 15 +   146 1?
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