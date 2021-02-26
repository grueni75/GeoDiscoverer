#!/bin/bash

# Get the raw data for Europe
echo Downloading Europe
#if false; then
rm -rf download
mkdir download
cd download
for y in 29 30 31 32 33 34 35; do
  echo -n "$y => "
  for x in I J K L M N O P Q R; do
    echo -n "$x "
    file=${x}${y}.zip
    skip=0
    if [ "$file" = "L29.zip" ]; then
      skip=1
    fi
    if [ "$file" = "P29.zip" ]; then
      file="FAR.zip"
    fi
    if [ "$file" = "Q29.zip" ]; then
      skip=1
    fi
    if [ "$file" = "R29.zip" ]; then
      file="JANMAYEN.zip"
    fi
    if [ "$file" = "Q29.zip" ]; then
      skip=1
    fi
    if [ "$file" = "R32.zip" ]; then
      skip=1
    fi
    if [ "$skip" = "0" ]; then
      if ! wget http://viewfinderpanoramas.org/dem3/$file >/dev/null 2>&1; then
        echo "Can not download $file.zip!"
      fi
    fi
  done
  echo 
done
#fi
#cd download

# Unpack the data
echo Unpacking data
error=0
for file in *.zip; do
  if ! unzip -o $file >/dev/null 2>&1; then
    echo "Can not unzip ${file}!"
    error=1
  fi
done
if [ "$error" = "1" ]; then
  echo "Errors occured!"
  exit 1
fi
mv */*.hgt .

# Create vrt file
echo Creating vrt file
find -name "*.hgt" >file.list
sed -i "s/\.\///" file.list
gdalbuildvrt -input_file_list file.list index.vrt

# Move data
echo Moving data
cd ..
mv download/*.hgt .
mv download/index.vrt .
rm -rf download



