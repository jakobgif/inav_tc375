#run this script inside aurix build folder otherwise aurix include paths wont be found
#get current path
BUILD_FOLDER=$PWD
#echo $BUILD_FOLDER

#convert inlcude path from aurix to unix path
echo creating include paths
sed -E -e 's#-I"C:([^"]+)"#-I"/mnt/c\1"#g' -e 's#\\#/#g' "AURIX_GCC_Compiler-Include_paths__-I_.opt" > includes.opt
INCLUDES=$(<includes.opt)
#echo $INCLUDES

#go to directory of this script
cd "$(dirname "$0")"

#compile definitions
DEFS="-DTC375"
#IMPORTANT: this can vary by machine
export CFLAGS="$INCLUDES -I/mnt/c/Infineon/AURIX-Studio-1.10.2/tools/Compilers/tricore-gcc11/tricore-elf/include $DEFS"
export SETTINGS_CXX="g++"

SETTINGS_FILE=../main/fc/settings.yaml
OUTPUT_DIR=$BUILD_FOLDER #../main/target

echo running settings.rb
ruby ./settings.rb \
    "../" \
    "$SETTINGS_FILE" \
    -o "$OUTPUT_DIR"

echo done