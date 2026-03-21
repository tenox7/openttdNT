This is a guide to compile strgen on gcc

Compile by typing
gcc strgen.c -o ../strgen.app -DUNIX

you now have a program called strgen.app in the ttd directory
go there by typing
cd ..

to make the english.lng file, type
./strgen.app lang/english.lng lang/english.txt

You now have english.lng in the folder lang
