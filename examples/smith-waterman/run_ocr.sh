EXEC=./smithwaterman.exe
INPUT_1=./string1-larger.txt
INPUT_2=./string2-larger.txt
TILE_WIDTH=290
TILE_HEIGHT=300

make clean; make compile
# Todo, update with machine descriptions
for MACH in 16 8 4 2 1; do
        echo "using $MACH"
        $EXEC  $INPUT_1 $INPUT_2 $TILE_WIDTH $TILE_HEIGHT | sed -e '/^Size.*/ d' -e '/^Imported.*/ d' -e '/^Allocat.*/ d' -e '/^Tile.*/ d'
done
