# |-----
# | Minitutorial
# |
# | Zerostrap is a concatenative stack-based language. Expressions are written in reverse polish notation:
5 5 @* # push 5, push 5, pop two stack items and feed them to @* and push the result
# | stack is now [25]
9000 @w # push 9000, pop as address, pop as value, *(address) = value (32-bit integer)
# | memory bytes 9000, 9001, 9002, and 9003 just got written to.
# | 9000 contains 25, the others contain 0.
# | 
# | Let's define and use a function, called asdf. ; is the end-of-function character.
# | Functions look like comments. If it's not a function (or label), always add a space after the # character of your comments!
#asdf 9000 @r ;
# | Now the function asdf exists, which reads the 32-bit integer at 9000 and pushes it to the stack.
asdf asdf @* 
# | Er, hmm. We forgot to write a writing macro. Our stack contains [125] now, so...
#asdf<- 9000 @w ;
asdf<-
# | OK, now we wrote 125 back to where asdf is.
# | By the way, take note that functions do not need to leave the stack at the same level. They're like macros in that way.
# | (The source code for zerostrap calls them macros and functions interchangeably.)
# | Functions are space-sensitive. Their uses must be followed by a space/tab/newline/etc.
# | Primitives (@-  @*  @/  @&  @,  @r  @w  ?  ;) and numbers are not space-sensitive.
# |
# | Let's do some control flow now.
# | Control flow uses the ? operator, which pops as test value, pops as label value, and jumps to label value if test value is nonzero.
555555 asdf ?
# SKIPPED
#555555
# | OK, now you know everything you need to know to write zerostrap code. Have fun!
# |-----


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

#scroll 200@r;
#scroll<- 200@w;

#99
  0 py<-
  #110
    0 px<-
    #120
    
      # *(557056 + py*320 + px) = (px % 64 + py % 64) / 2 + 16 + scroll * 4
      px  scroll 4 * +    557056 py 320 * + px +    w8

      # px += 1
      px 1 + px<-
      # loop until px is 320
    120 px 320 < ?
    
    py 1 + py<-
  110 py 200 < ?
  
  # go again with another scroll value
  scroll 1 + 20 % scroll<-
99 1?

終
