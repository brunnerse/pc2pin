add_library(SerialCpp STATIC SimpleSerial.cpp)

target_compile_features(SerialCpp PRIVATE cxx_std_11)

target_include_directories(SerialCpp 
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


