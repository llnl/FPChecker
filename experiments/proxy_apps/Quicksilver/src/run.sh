#!/bin/bash -x

# ./qs -X 5 -Y 5 -Z 5 -x 20 -y 20 -z 15 -N 3 -n 5
# ./qs -X 1 -Y 1 -Z 1 -x 1 -y 1 -z 1 -N 1 -n 20 -g 20 -I 1 -J 1 -K 1 -e cancellation_spectrum
./qs -X 1 -Y 1 -Z 1 -x 1 -y 1 -z 1 -N 3 -n 25 -g 25 -I 1 -J 1 -K 1

# ./qs -i quicksilver_cancellation.inp