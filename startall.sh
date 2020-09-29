
make clean
make hellox6 && make 3d && make 3d2 && make graph && make pong

./pong &
./graph &
./3d &
./hellox6 &
./3d2 &
DBX_DISP_DIV=1 ./3d3 kernel2.c &
