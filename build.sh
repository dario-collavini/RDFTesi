#!/bin/bash
echo "Building project..."
sudo cp ./lib/CppRDFox/libCppRDFox.so /usr/local/lib
sudo mkdir /usr/local/include/CppRDFox
sudo cp ./include/RDFoxDataStore.h /usr/local/include/CppRDFox
g++ -o Main main.cpp TestRDFListener.cpp ./RDFStore/RDFStore.cpp ./RDFStore/EventConverter.cpp ./Parser/RDFTESLABaseListener.cpp ./Parser/RDFTESLALexer.cpp ./Parser/RDFTESLAParser.cpp ./Parser/RDFTESLAListener.cpp ./Parser/RDFTRexRuleParser.cpp ./RDFConstructor/RDFConstructor.cpp ./RDFConstructor/TRexListener.cpp -lTRex2 -lCppRDFox -L./lib/antlr -lantlr4-runtime  -I./include -I./include/antlr4-runtime -std=c++11
echo "Done!"

