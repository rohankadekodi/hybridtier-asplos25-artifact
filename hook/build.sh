#g++ -shared -fPIC hook.cpp -o hook.so -O3 -ldl -lpthread -lnuma
#g++ -shared -fPIC hook.cpp -o hook.so -O3 -ldl -lpthread -lnuma -DTARGET_EXE_NAME=\"cachebench\"
g++ -shared -fPIC hook.cpp -o hook.so -O3 -ldl -lpthread -lnuma -DTARGET_EXE_NAME=\"cachebench\" -DHYBRIDTIER_REGULAR
#g++ -shared -fPIC hook.cpp -o hook.so -O3 -ldl -lpthread -lnuma -g -fno-omit-frame-pointer


