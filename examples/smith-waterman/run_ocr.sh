EXEC=./smithwaterman
INPUT_1=./string1-larger.txt
INPUT_2=./string2-larger.txt
TILE_WIDTH=290
TILE_HEIGHT=300

for MACH in 16 8 4 2 1; do
        echo "using $MACH"
        $EXEC -md mach_$MACH.xml $INPUT_1 $INPUT_2 $TILE_WIDTH $TILE_HEIGHT | sed -e '/^Size.*/ d' -e '/^Imported.*/ d' -e '/^Allocat.*/ d' -e '/^Tile.*/ d'
done
