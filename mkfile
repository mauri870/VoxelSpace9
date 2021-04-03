</$objtype/mkfile

ALL=demo

all:V: $ALL

%: %.$O
	$LD -o $target $prereq

%.$O: %.c
	$CC $CFLAGS $stem.c

clean:V:
    rm -f $ALL *.[$OS]
