#0

# ----
# Basic language bootstrap macros
# ----

#r 0 98304@-@-@r;
#w 0 98304@-@-@w;
#- @-;
#& @&;
#/ @/;
#* @*;
#SW 24w 20w 24r 20r ;
#!~ 0 1 - SW - ;
#neg 0 SW - ;
#+ neg - ;
#| !~ SW !~ & !~ ;

# *(int*)0 = INT_MIN
2048 2048 512 * * 0w

#ltz 0r & 0r / 1 & ;
#_not 1 SW - ;
#< 44w 40w 40r 44r - ltz 40r ltz 44r ltz _not & | ;
#>= < _not ;

#w8 160w 164w 160r 1+ r 165w 164r 160r w ;
#r8 r 255 & ;

0 100w
4 1? # skip over definitions
#.
    655360 100r + w8
    15 655361 100r + w8
    100r 2 + 100w
    100r 100r 3840 < neg & 100w
;
#callstk_top 0@C 4 - ;
#print_rest_of_line
    callstk_top 4 - @r 3 + 98304 - 50w
    #510
        50r r8 .
        50r 1 + 50w
    510 50r r8 32 >= ?
;
#4

# ----
# Okay! Time for the actual program.
# ----

print_rest_of_line # Hello, world!

# infinite loop to end program "safely"
#7
7 1?
終