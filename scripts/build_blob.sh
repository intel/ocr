# Generate the CE blob
../build/x86-builder-fsim-ce/builder.exe -ocr:cfg fsim_ce.cfg

# Generate the XE blob - TODO
#../build/x86-builder-fsim-xe/builder.exe -ocr:cfg fsim_xe.cfg

# Combine the two
./aggregate_binary_files.sh ce_blob.bin ce_blob.bin dummy ../../ss/xe-sim/tests/mb-blob/blob
