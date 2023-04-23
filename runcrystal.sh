#!/bin/bash

SAVE_DIR="states"
NO_SAVE="$1"

SAVE_FLAG=""


if [ -z "$NO_SAVE" ]
  then
    max=$(ls -1 "$SAVE_DIR" | grep "[0-9]" | sed "s/[^0-9]//g" | sort -rn | head -1)
    newMax="$(($max + 1))"

    # Create a backup of current save
    cp "$SAVE_DIR/crystal.state" "$SAVE_DIR/crystal$newMax.state"
    SAVE_FILE="$SAVE_DIR/crystal.state"
    SAVE_FLAG="-d $SAVE_FILE"
fi


echo "$SAVE_FLAG"

# start emulator to read and write this state
./nanoarch\
  ../gambatte-libretro/gambatte_libretro.so\
  ../pokecrystal/pokecrystal.gbc\
  -l "$SAVE_DIR/crystal.state"\
  $SAVE_FLAG
